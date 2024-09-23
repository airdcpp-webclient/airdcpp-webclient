/*
* Copyright (C) 2011-2024 AirDC++ Project
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef DCPLUSPLUS_WEBSERVER_HTTP_MANAGER_H
#define DCPLUSPLUS_WEBSERVER_HTTP_MANAGER_H

#include "stdinc.h"

#include "FileServer.h"

#include "HttpRequest.h"
#include "HttpUtil.h"
#include "WebServerManager.h"
#include "WebUserManager.h"

#include <airdcpp/format.h>
#include <airdcpp/AppUtil.h>
#include <airdcpp/Util.h>


namespace webserver {
	class HttpManager {
	public:
		explicit HttpManager(WebServerManager* aWsm) noexcept : wsm(aWsm) {}

		const FileServer& getFileServer() const noexcept {
			return fileServer;
		}

		HttpManager(HttpManager&) = delete;
		HttpManager& operator=(HttpManager&) = delete;

		template<class T>
		void setEndpointHandlers(T& aEndpoint, bool aIsSecure) {
			aEndpoint.set_http_handler(
			 	std::bind_front(&HttpManager::handleHttpRequest<T>, this, &aEndpoint, aIsSecure));
		}

		void start(const string& aWebResourcePath) noexcept;
		void stop() noexcept;
	private:
		static api_return handleApiRequest(const HttpRequest& aRequest,
			json& output_, json& error_, const ApiDeferredHandler& aDeferredHandler) noexcept;

		// Returns false in case of invalid token format
		template <typename ConnType>
		bool getOptionalHttpSession(const ConnType& con, const string& aIp, SessionPtr& session_) {
			auto authToken = HttpUtil::parseAuthToken(con->get_request());
			if (authToken != websocketpp::http::empty_header) {
				try {
					session_ = wsm->getUserManager().parseHttpSession(authToken, aIp);
				} catch (const std::exception& e) {
					con->set_body(e.what());
					con->set_status(websocketpp::http::status_code::unauthorized);
					return false;
				}
			}

			return true;
		}

		template <typename EndpointType, typename ConnType>
		void handleHttpApiRequest(const HttpRequest& aRequest, EndpointType* s, const ConnType& con) {
			wsm->onData(aRequest.path + ": " + aRequest.httpRequest.get_body(), TransportType::TYPE_HTTP_API, Direction::INCOMING, aRequest.ip);

			// Don't capture aRequest in here (it can't be used for async actions)
			auto responseF = [this, s, con, ip = aRequest.ip](websocketpp::http::status_code::value aStatus, const json& aResponseJsonData, const json& aResponseErrorJson) {
				string data;
				const auto& responseJson = !aResponseErrorJson.is_null() ? aResponseErrorJson : aResponseJsonData;
				if (!responseJson.is_null()) {
					try {
						data = responseJson.dump();
					} catch (const std::exception& e) {
						WebServerManager::logDebugError(s, "Failed to convert data to JSON: " + string(e.what()), websocketpp::log::elevel::fatal);

						con->set_body("Failed to convert data to JSON: " + string(e.what()));
						con->set_status(websocketpp::http::status_code::internal_server_error);
						return;
					}
				}

				wsm->onData(con->get_resource() + " (" + Util::toString(aStatus) + "): " + data, TransportType::TYPE_HTTP_API, Direction::OUTGOING, ip);

				con->set_body(data);
				con->append_header("Content-Type", "application/json");
				con->append_header("Connection", "close"); // Workaround for https://github.com/zaphoyd/websocketpp/issues/890
				con->set_status(aStatus);
			};


			bool isDeferred = false;
			const auto deferredF = [&isDeferred, &responseF, con]() {
				con->defer_http_response();
				isDeferred = true;

				return [con, cb = std::move(responseF)](websocketpp::http::status_code::value aStatus, const json& aResponseJsonData, const json& aResponseErrorJson) {
					cb(aStatus, aResponseJsonData, aResponseErrorJson);
					con->send_http_response();
				};
			};

			json output, apiError;
			auto status = handleApiRequest(
				aRequest,
				output,
				apiError,
				deferredF
			);

			if (!isDeferred) {
				responseF(status, output, apiError);
			}
		}

		template <typename ConnType>
		void handleHttpFileRequest(const HttpRequest& aRequest, const ConnType& con) {
			wsm->onData(aRequest.httpRequest.get_method() + " " + aRequest.path, TransportType::TYPE_HTTP_FILE, Direction::INCOMING, aRequest.ip);

			StringPairList headers;
			std::string output;

			// Don't capture aRequest in here (it can't be used for async actions)
			auto responseF = [this, con, ip = aRequest.ip](websocketpp::http::status_code::value aStatus, const string& aOutput, const StringPairList& aHeaders = StringPairList()) {
				wsm->onData(
					con->get_request().get_method() + " " + con->get_resource() + ": " + Util::toString(aStatus) + " (" + Util::formatBytes(aOutput.length()) + ")",
					TransportType::TYPE_HTTP_FILE,
					Direction::OUTGOING,
					ip
				);

				con->append_header("Connection", "close"); // Workaround for https://github.com/zaphoyd/websocketpp/issues/890

				if (HttpUtil::isStatusOk(aStatus)) {
					// Don't set any incomplete/invalid headers in case of errors...
					for (const auto& [name, value] : aHeaders) {
						con->append_header(name, value);
					}

					con->set_status(aStatus);
					con->set_body(aOutput);
				} else {
					con->set_status(aStatus, aOutput);
					con->set_body(aOutput);
				}
			};

			bool isDeferred = false;
			const auto deferredF = [&isDeferred, &responseF, con]() {
				con->defer_http_response();
				isDeferred = true;

				return [cb = std::move(responseF), con](websocketpp::http::status_code::value aStatus, const string& aOutput, const StringPairList& aHeaders) {
					cb(aStatus, aOutput, aHeaders);
					con->send_http_response();
				};
			};

			auto status = fileServer.handleRequest(aRequest, output, headers, deferredF);
			if (!isDeferred) {
				responseF(status, output, headers);
			}
		}

		template <typename EndpointType>
		void handleHttpRequest(EndpointType* s, bool aIsSecure, websocketpp::connection_hdl hdl) {
			// Blocking HTTP Handler
			auto con = s->get_con_from_hdl(hdl);
			auto ip = con->get_raw_socket().remote_endpoint().address().to_string();

			// We also have public resources (such as UI resources and auth endpoints) 
			// so session isn't required at this point
			SessionPtr session = nullptr;
			if (!getOptionalHttpSession(con, ip, session)) {
				return;
			}

			HttpRequest request{ session, ip, con->get_resource(), con->get_request(), aIsSecure };
			if (request.path.length() >= 4 && request.path.compare(0, 4, "/api") == 0) {
				handleHttpApiRequest(request, s, con);
			} else {
				handleHttpFileRequest(request, con);
			}
		}

		WebServerManager* wsm;
		FileServer fileServer;
	};
}

#endif // DCPLUSPLUS_DCPP_WEBSERVER_H