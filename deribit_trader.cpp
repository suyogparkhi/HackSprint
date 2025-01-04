#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>
#include <thread>
#include <ctime>

#include <libwebsockets.h>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

class DeribitTrader {
public:
    struct OrderRequest {
        std::string instrument_name;
        std::string direction;  // "buy" or "sell"
        double amount;
        double price;
        std::string type;      // "limit", "market", etc.
        bool post_only{false};
        bool reduce_only{false};
        std::string time_in_force{"good_til_cancelled"};
    };

    struct OpenOrder {
        std::string order_id;
        std::string instrument_name;
        std::string direction;
        double price;
        double amount;
        std::string order_type;
        std::string order_state;
        std::string time_in_force;
    };

private:
    // API credentials
    std::string ws_access_token_;
    std::string refresh_token_;
    std::string api_key_;
    std::string api_secret_;
    std::string base_url_{"https://test.deribit.com/api/v2"};
    std::string ws_url_{"wss://test.deribit.com/ws/api/v2"};
    
    // WebSocket connection
    struct lws_context* ws_context_{nullptr};
    struct lws* ws_connection_{nullptr};
    
    // Callback handlers
    std::map<std::string, std::function<void(const json&)>> message_handlers_;
    
    // Authentication token
    std::string access_token_;
    int64_t token_expiry_{0};

    // WebSocket protocols
    static struct lws_protocols* get_protocols() {
        static struct lws_protocols protocols[] = {
            {
                "deribit-protocol",
                DeribitTrader::ws_callback,    // Callback function
                0,              // Per session data size
                4096,           // Rx buffer size
            },
            { 0 }  // Terminating null protocol
        };
        return protocols;
    }

    // Connect to WebSocket
    void connect_websocket() {
        struct lws_client_connect_info i;
        memset(&i, 0, sizeof(i));
        
        i.context = ws_context_;
        i.port = 443;
        i.address = "test.deribit.com";
        i.path = "/ws/api/v2";
        i.host = i.address;
        i.origin = i.address;
        i.protocol = get_protocols()[0].name;
        i.pwsi = &ws_connection_;
        i.userdata = this;
        
        i.ssl_connection = 
            LCCSCF_USE_SSL |           
            LCCSCF_ALLOW_SELFSIGNED |  
            LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK; 

        spdlog::info("Attempting WebSocket connection to {}:{}{}", 
                     i.address, i.port, i.path);

        ws_connection_ = lws_client_connect_via_info(&i);
        
        if (!ws_connection_) {

            char errbuf[256];
            ERR_error_string_n(ERR_get_error(), errbuf, sizeof(errbuf));
            
            std::string error_msg = "Failed to establish WebSocket connection. SSL Error: " + 
                                    std::string(errbuf);
            spdlog::error(error_msg);
            throw std::runtime_error(error_msg);
        }
        
        int n = lws_service(ws_context_, 5000);
        if (n < 0) {
            spdlog::error("Initial WebSocket service failed with code {}", n);
            throw std::runtime_error("WebSocket service initialization failed");
        }
    }
    
public:
    DeribitTrader(const std::string& api_key, const std::string& api_secret)
        : api_key_(api_key), api_secret_(api_secret) {
        init_websocket();
        authenticate();
    }

    ~DeribitTrader() {
        cleanup();
    }


    struct lws_context* get_ws_context() { return ws_context_; }


    json get_instrument_details(const std::string& instrument_name) {
        json params = {
            {"jsonrpc", "2.0"},
            {"method", "public/get_instrument"},
            {"params", {
                {"instrument_name", instrument_name}
            }},
            {"id", 1}
        };

        return send_public_request("/public/get_instrument", params["params"]);
    }


    double round_to_contract_size(const std::string& instrument_name, double amount) {
        try {

            json instrument_response = get_instrument_details(instrument_name);
            
            if (!instrument_response.contains("result")) {
                std::cerr << "Failed to retrieve instrument details" << std::endl;
                return amount;
            }

            const auto& instrument = instrument_response["result"];
            
            double contract_size = instrument.value("contract_size", 1.0);
            
            double rounded_amount = std::round(amount / contract_size) * contract_size;
            
            std::cout << "Original amount: " << amount 
                      << ", Contract size: " << contract_size 
                      << ", Rounded amount: " << rounded_amount << std::endl;
            
            return rounded_amount;
        } catch (const std::exception& e) {
            std::cerr << "Error rounding to contract size: " << e.what() << std::endl;
            return amount;
        }
    }

    double get_minimum_order_amount(const std::string& instrument_name) {
        try {

            json instrument_response = get_instrument_details(instrument_name);

            if (!instrument_response.contains("result")) {
                std::cerr << "Failed to retrieve instrument details" << std::endl;
                return 0.001; 
            }

            const auto& instrument = instrument_response["result"];
            
            double min_amount = 0.001; 
            
            if (instrument.contains("min_order_size")) {
                min_amount = instrument["min_order_size"].get<double>();
            } else if (instrument.contains("contract_size")) {
                min_amount = instrument["contract_size"].get<double>();
            }
            
            std::cout << "Minimum order amount for " << instrument_name 
                      << ": " << min_amount << std::endl;
            
            return min_amount;
        } catch (const std::exception& e) {
            std::cerr << "Error determining minimum order amount: " << e.what() << std::endl;
            return 0.001; 
        }
    }

    std::string place_order(const OrderRequest& request) {


        if (request.instrument_name.empty()) {
            throw std::invalid_argument("Instrument name is required");
        }

        double order_amount = request.amount;
        double min_amount = get_minimum_order_amount(request.instrument_name);
        
        if (order_amount <= 0) {
            order_amount = min_amount;
            std::cout << "Adjusted order amount to minimum: " << order_amount << std::endl;
        } else if (order_amount < min_amount) {
            order_amount = min_amount;
            std::cout << "Rounded order amount to minimum: " << order_amount << std::endl;
        }

        double rounded_amount = round_to_contract_size(request.instrument_name, order_amount);

        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "private/" + request.direction},
            {"params", {
                {"instrument_name", request.instrument_name},
                {"type", request.type.empty() ? "limit" : request.type},
                {"amount", rounded_amount}
            }},
            {"id", 1}
        };

        if (request.type == "limit" || request.type.empty()) {
            if (request.price <= 0) {
                throw std::invalid_argument("Limit order price must be positive");
            }
            payload["params"]["price"] = request.price;
        }

        if (request.post_only) {
            payload["params"]["post_only"] = true;
        }

        if (request.reduce_only) {
            payload["params"]["reduce_only"] = true;
        }

        payload["params"]["time_in_force"] = request.time_in_force.empty() 
            ? "good_til_cancelled" 
            : request.time_in_force;

        std::cout << "Order Placement Payload:" << std::endl;
        std::cout << payload.dump(4) << std::endl;


        std::string endpoint = "/private/" + request.direction;

        try {

            json response = send_authenticated_request(endpoint, payload);
  
            std::cout << "Order Placement Response:" << std::endl;
            std::cout << response.dump(4) << std::endl;

            if (response.contains("error")) {
                std::string error_msg = "Order placement failed: " + 
                    response["error"]["message"].get<std::string>();
                

                if (response["error"].contains("code")) {
                    error_msg += " (Code: " + 
                        std::to_string(response["error"]["code"].get<int>()) + ")";
                }
                
                if (response["error"].contains("data")) {
                    const auto& error_data = response["error"]["data"];
                    error_msg += "\nDetails: " + error_data.dump();
                }
                
                throw std::runtime_error(error_msg);
            }


            if (!response.contains("result") || 
                !response["result"].contains("order") ||
                !response["result"]["order"].contains("order_id")) {
                throw std::runtime_error("Invalid order response structure");
            }

            std::string order_id = response["result"]["order"]["order_id"].get<std::string>();


            std::cout << "Successfully placed order:" << std::endl;
            std::cout << "Order ID: " << order_id << std::endl;
            std::cout << "Instrument: " << request.instrument_name << std::endl;
            std::cout << "Direction: " << request.direction << std::endl;
            std::cout << "Amount: " << rounded_amount << std::endl;

            return order_id;

        } catch (const std::exception& e) {

            std::cerr << "Order Placement Exception: " << e.what() << std::endl;
            throw;
        }
    }

    bool cancel_order(const std::string& order_id) {

        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "private/cancel"},
            {"params", {
                {"order_id", order_id}
            }},
            {"id", 1}
        };


        try {
            json response = send_authenticated_request("/private/cancel", payload);
            
            std::cout << "Order Cancellation Response:" << std::endl;
            std::cout << response.dump(4) << std::endl;

            if (response.contains("error")) {
                throw std::runtime_error("Order cancellation failed: " + 
                    response["error"]["message"].get<std::string>());
            }

            return response.contains("result") && !response["result"].is_null();
        } catch (const std::exception& e) {
            std::cerr << "Error canceling order: " << e.what() << std::endl;
            return false;
        }
    }

    std::vector<OpenOrder> get_open_orders(const std::string& instrument_name = "") {

        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "private/get_open_orders"},
            {"params", json::object()},
            {"id", 1}
        };

        if (!instrument_name.empty()) {
            payload["params"]["instrument_name"] = instrument_name;
        }

        try {
            json response = send_authenticated_request("/private/get_open_orders", payload);
            
            std::cout << "Open Orders Response:" << std::endl;
            std::cout << response.dump(4) << std::endl;

            if (response.contains("error")) {
                throw std::runtime_error("Failed to fetch open orders: " + 
                    response["error"]["message"].get<std::string>());
            }

            std::vector<OpenOrder> open_orders;
            
            if (response.contains("result") && !response["result"].is_null()) {
                for (const auto& order : response["result"]) {
                    open_orders.push_back({
                        order["order_id"].get<std::string>(),
                        order["instrument_name"].get<std::string>(),
                        order["direction"].get<std::string>(),
                        order["price"].get<double>(),
                        order["amount"].get<double>(),
                        order["order_type"].get<std::string>(),
                        order["order_state"].get<std::string>(),
                        order.value("time_in_force", "good_til_cancelled")
                    });
                }
            }

            return open_orders;
        } catch (const std::exception& e) {
            std::cerr << "Error fetching open orders: " << e.what() << std::endl;
            return {};
        }
    }

    bool modify_order(const std::string& order_id, double new_amount, double new_price, 
                      const std::string& advanced = "") {

        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "private/edit"},
            {"params", {
                {"order_id", order_id},
                {"amount", new_amount},
                {"price", new_price}
            }},
            {"id", 1}
        };

        if (!advanced.empty()) {
            payload["params"]["advanced"] = advanced;
        }

        try {
            json response = send_authenticated_request("/private/edit", payload);
            
            std::cout << "Order Modification Response:" << std::endl;
            std::cout << response.dump(4) << std::endl;

            if (response.contains("error")) {
                throw std::runtime_error("Order modification failed: " + 
                    response["error"]["message"].get<std::string>());
            }

            return response.contains("result") && !response["result"].is_null();
        } catch (const std::exception& e) {
            std::cerr << "Error modifying order: " << e.what() << std::endl;
            return false;
        }
    }


    json get_orderbook(const std::string& instrument_name, int depth = 20) {
        json params = {
            {"instrument_name", instrument_name},
            {"depth", depth}
        };
        
        return send_public_request("/public/get_order_book", params);
    }


    void subscribe_orderbook(const std::string& instrument_name) {
        json msg = {
            {"jsonrpc", "2.0"},
            {"method", "public/subscribe"},
            {"params", {
                {"channels", {"book." + instrument_name + ".100ms"}}
            }},
            {"id", 42}
        };
        
        send_ws_message(msg.dump());
    }

    void subscribe_trades(const std::string& instrument_name) {
        json msg = {
            {"jsonrpc", "2.0"},
            {"method", "public/subscribe"},
            {"params", {
                {"channels", {"trades." + instrument_name + ".100ms"}}
            }},
            {"id", 43}
        };
        
        send_ws_message(msg.dump());
    }

private:

    void init_ssl() {
        //OpenSSL initialization for multi-threaded applications
        static std::once_flag init_flag;
        std::call_once(init_flag, []() {
            OpenSSL_add_all_algorithms();
            SSL_load_error_strings();
            SSL_library_init();
            
                static std::vector<std::mutex> ssl_mutexes(CRYPTO_num_locks());
                CRYPTO_set_locking_callback([](int mode, int type, const char* file, int line) {
                    if (mode & CRYPTO_LOCK) {
                        ssl_mutexes[type].lock();
                    } else {
                        ssl_mutexes[type].unlock();
                    }
                });
           
        });
    }


    void init_websocket() {

        init_ssl();
        struct lws_context_creation_info info;
        memset(&info, 0, sizeof(info));
        

        info.options = 
            LWS_SERVER_OPTION_VALIDATE_UTF8 |
            LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        
        info.port = CONTEXT_PORT_NO_LISTEN;
        info.protocols = get_protocols();
        info.gid = -1;
        info.uid = -1;
        info.user = this;  


        info.ssl_cert_filepath = nullptr;
        info.ssl_private_key_filepath = nullptr;

        lws_set_log_level(
            LLL_ERR | LLL_WARN, 
            nullptr
        );

        ws_context_ = lws_create_context(&info);
        if (!ws_context_) {
            throw std::runtime_error("Failed to create WebSocket context with SSL");
        }

        // Try to connect WebSocket
        try {
            connect_websocket();
        } catch (const std::exception& e) {
            spdlog::error("WebSocket connection failed: {}", e.what());
            throw;
        }
    }


    // Authenticate with Deribit API
   void authenticate() {

        json auth_params = {
            {"grant_type", "client_credentials"},
            {"client_id", api_key_},
            {"client_secret", api_secret_}
        };

        json response = send_public_request("/public/auth", auth_params);

        std::cout << "Authentication Response: " << response.dump(4) << std::endl;

        if (response.contains("error")) {
            throw std::runtime_error("Authentication failed: " + 
                response["error"]["message"].get<std::string>());
        }

        if (!response.contains("result")) {
            throw std::runtime_error("Invalid authentication response: no result field");
        }

        const auto& result = response["result"];

        if (!result.contains("access_token") || 
            !result.contains("expires_in") || 
            result["access_token"].is_null()) {
            throw std::runtime_error("Invalid authentication response: missing critical fields");
        }

        access_token_ = result["access_token"].get<std::string>();

        double expires_in = result["expires_in"].is_number() 
            ? result["expires_in"].get<double>() 
            : 0.0;

        token_expiry_ = std::time(nullptr) + static_cast<long>(expires_in);

        std::cout << "Successfully authenticated." << std::endl;
        std::cout << "Token expires in: " << expires_in << " seconds" << std::endl;
        std::cout << "Scope: " << (result.contains("scope") ? result["scope"].get<std::string>() : "N/A") << std::endl;


        if (result.contains("refresh_token")) {
            refresh_token_ = result["refresh_token"].get<std::string>();
            std::cout << "Refresh token obtained" << std::endl;
        }
    }

    json send_public_request(const std::string& endpoint, const json& params) {
        CURL* curl = curl_easy_init();
        std::string response_string;

        if (curl) {
            std::string url = base_url_ + endpoint;
            

            json rpc_payload = {
                {"jsonrpc", "2.0"},
                {"method", endpoint.substr(1)},  
                {"id", 1},
                {"params", params}
            };

            std::string post_data = rpc_payload.dump();

            // std::cout << "Sending request to: " << url << std::endl;
            // std::cout << "Request payload: " << post_data << std::endl;

            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

            curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);

            CURLcode res = curl_easy_perform(curl);
            
            // Clean up
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            if (res != CURLE_OK) {
                throw std::runtime_error("Failed to send request: " + 
                    std::string(curl_easy_strerror(res)));
            }

            // std::cout << "Raw Response: " << response_string << std::endl;

            return json::parse(response_string);
        }

        throw std::runtime_error("Failed to initialize CURL");
    }

    // Helper for sending authenticated HTTP requests
    json send_authenticated_request(const std::string& endpoint, const json& params) {
        if (std::time(nullptr) >= token_expiry_) {
            authenticate();
        }

        CURL* curl = curl_easy_init();
        std::string response_string;

        if (curl) {
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, ("Authorization: Bearer " + access_token_).c_str());
            headers = curl_slist_append(headers, "Content-Type: application/json");

            std::string url = base_url_ + endpoint;
            std::string post_data = params.dump();

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

            CURLcode res = curl_easy_perform(curl);
            
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            if (res != CURLE_OK) {
                throw std::runtime_error("Failed to send request: " + std::string(curl_easy_strerror(res)));
            }
        }

        return json::parse(response_string);
    }

    static int ws_callback(struct lws* wsi, enum lws_callback_reasons reason,
                          void* user, void* in, size_t len) {
        if (!wsi) return -1;
        DeribitTrader* instance = static_cast<DeribitTrader*>(
            lws_context_user(lws_get_context(wsi)));
        if (!instance) return -1;

        switch (reason) {
            case LWS_CALLBACK_CLIENT_ESTABLISHED:
                instance->on_ws_connect();
                break;

            case LWS_CALLBACK_CLIENT_RECEIVE:
                instance->on_ws_message(std::string(static_cast<char*>(in), len));
                break;

            case LWS_CALLBACK_CLIENT_CLOSED:
                instance->on_ws_close();
                break;

            default:
                break;
        }

        return 0;
    }

void on_ws_connect() {
        spdlog::info("WebSocket connected");
    
        json auth_msg = {
            {"jsonrpc", "2.0"},
            {"method", "public/auth"},
            {"id", 1},
            {"params", {
                {"grant_type", "client_credentials"},
                {"client_id", api_key_},
                {"client_secret", api_secret_}
            }}
        };
        
        send_ws_message(auth_msg.dump());
    }

void on_ws_message(const std::string& message) {
        try {

            // std::cout << "Raw WebSocket message received: " << message << std::endl;

            if (message.empty()) {
                spdlog::warn("Received empty WebSocket message");
                return;
            }

            json j;
            try {
                j = json::parse(message);
            } catch (const json::parse_error& e) {
                spdlog::error("JSON parsing error: {}", e.what());
                spdlog::error("Problematic message: {}", message);
                return;
            }

           //  std::cout << "Parsed WebSocket message: " << j.dump(4) << std::endl;
            
            log_message_structure(j);

            if (j.contains("method")) {
                std::string method = j["method"].get<std::string>();
                
                if (method == "public/auth") {
                    handle_ws_authentication(j);
                    return;
                }
            }
            
            if (j.contains("result")) {
                handle_ws_result(j);
            }
            
            if (j.contains("error")) {
                handle_ws_error(j);
            }
        } catch (const std::exception& e) {
            spdlog::error("Exception in WebSocket message handling: {}", e.what());
            std::cerr << "Problematic message: " << message << std::endl;
        }
    }

    void log_message_structure(const json& message) {
        std::cout << "Message Keys: ";
        for (const auto& [key, value] : message.items()) {
            std::cout << key << " ";
        }
        std::cout << std::endl;

        if (message.contains("method")) {
            std::cout << "Method: " << message["method"].get<std::string>() << std::endl;
        }
        
        if (message.contains("result")) {
            std::cout << "Result type: " 
                      << (message["result"].is_null() ? "null" : 
                          message["result"].type_name()) 
                      << std::endl;
        }
        
        if (message.contains("error")) {
            std::cout << "Error details: " << message["error"].dump(4) << std::endl;
        }
    }

    void handle_ws_authentication(const json& auth_response) {
        try {

            if (!auth_response.contains("result") || auth_response["result"].is_null()) {
                spdlog::error("WebSocket authentication failed: No result or result is null");
                
                std::cout << "Full auth response: " 
                          << auth_response.dump(4) << std::endl;
                return;
            }

            const auto& result = auth_response["result"];     

            std::cout << "Authentication Result: " << result.dump(4) << std::endl;

            if (!result.contains("access_token") || result["access_token"].is_null()) {
                spdlog::error("WebSocket authentication failed: No access token");
                return;
            }

            // Update WebSocket-specific access token 
            ws_access_token_ = result["access_token"].get<std::string>();
            
            spdlog::info("WebSocket authentication successful");
            
            // std::cout << "WebSocket Access Token: " << ws_access_token_ << std::endl;
            
            subscribe_default_channels();
        } catch (const std::exception& e) {
            spdlog::error("Error processing WebSocket authentication: {}", e.what());
        }
    }

    void handle_ws_result(const json& result_response) {
        try {
            if (!result_response.contains("result")) {
                spdlog::warn("Received result response without 'result' field");
                return;
            }

            const auto& result = result_response["result"];
            
            std::cout << "Received WebSocket result: " 
                      << (result.is_null() ? "NULL" : result.dump(4)) << std::endl;

            // Handle specific result types or actions
            if (result.is_object()) {
                for (const auto& [key, value] : result.items()) {
                    std::cout << "Result key: " << key 
                              << ", Value type: " << value.type_name() << std::endl;
                }
            }
        } catch (const std::exception& e) {
            spdlog::error("Error handling WebSocket result: {}", e.what());
        }
    }

    void handle_ws_error(const json& error_response) {
        try {
            if (!error_response.contains("error")) {
                spdlog::warn("Received error response without 'error' field");
                return;
            }

            const auto& error = error_response["error"];

            spdlog::error("Received WebSocket error:");
            
            if (error.is_object()) {
                if (error.contains("code")) {
                    spdlog::error("Error Code: {}", error["code"].get<int>());
                }
                
                if (error.contains("message")) {
                    spdlog::error("Error Message: {}", error["message"].get<std::string>());
                }
                
                std::cout << "Full error details: " << error.dump(4) << std::endl;
            } else {
                std::cout << "Error details: " << error.dump(4) << std::endl;
            }
        } catch (const std::exception& e) {
            spdlog::error("Error handling WebSocket error response: {}", e.what());
        }
    }


    void subscribe_default_channels() {
        try {
            subscribe_orderbook("BTC-PERPETUAL");
            subscribe_trades("BTC-PERPETUAL");
        } catch (const std::exception& e) {
            spdlog::error("Error subscribing to default channels: {}", e.what());
        }
    }

    void on_ws_close() {
        spdlog::info("WebSocket connection closed");
        // TODO: Reconnect or handle close event
    }

    void send_ws_message(const std::string& message) {
        if (!ws_connection_) return;

        // LWS_PRE padding as required by libwebsockets
        std::vector<uint8_t> buf(LWS_PRE + message.length());
        memcpy(buf.data() + LWS_PRE, message.c_str(), message.length());

        lws_write(ws_connection_, buf.data() + LWS_PRE, message.length(), LWS_WRITE_TEXT);
    }

    static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    void cleanup() {
        if (ws_connection_) {
            lws_callback_on_writable(ws_connection_);
            lws_cancel_service(ws_context_);
        }
        
        if (ws_context_) {
            lws_context_destroy(ws_context_);
            ws_context_ = nullptr;
        }
    }
};


class TradingApp {
private:
    DeribitTrader& trader_;
    bool running_ = true;
    std::string current_instrument_ = "BTC-PERPETUAL";


    struct MarketSnapshot {
        double best_bid = 0.0;
        double best_ask = 0.0;
        double last_price = 0.0;
        double high_24h = 0.0;
        double low_24h = 0.0;
        double volume = 0.0;
        std::chrono::steady_clock::time_point last_updated;
    };
    MarketSnapshot market_data_;

public:
    TradingApp(DeribitTrader& trader) : trader_(trader) {}

    void run() {
        // Start market data subscription thread
        std::thread market_data_thread(&TradingApp::update_market_data, this);
        market_data_thread.detach();

        while (running_) {
            display_main_menu();
        }
    }

private:
    void display_main_menu() {
        clear_screen();
        display_market_overview();

        std::cout << "\n===== Deribit Trading Terminal =====\n";
        std::cout << "Current Instrument: " << current_instrument_ << "\n\n";
        std::cout << "1. Place Order\n";
        std::cout << "2. Cancel Order\n";
        std::cout << "3. Modify Order\n";
        std::cout << "4. Get Open Orders\n";
        std::cout << "5. View Order History\n";
        std::cout << "6. Change Instrument\n";
        std::cout << "7. Real-time Market Analysis\n";
        std::cout << "8. Exit\n";
        std::cout << "Enter your choice: ";

        int choice;
        std::cin >> choice;

        switch (choice) {
            case 1: place_order(); break;
            case 2: cancel_order(); break;
            case 3: modify_order(); break;
            case 4: get_open_orders(); break;
            case 5: view_order_history(); break;
            case 6: change_instrument(); break;
            case 7: real_time_market_analysis(); break;
            case 8: running_ = false; break;
            default: 
                std::cout << "Invalid choice. Press Enter to continue...";
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cin.get();
        }
    }

    void place_order() {
        clear_screen();
        std::cout << "Place Order for " << current_instrument_ << "\n";
        
        std::string direction;
        double amount, price;
        std::string type;

        std::cout << "Direction (buy/sell): ";
        std::cin >> direction;

        std::cout << "Amount: ";
        std::cin >> amount;

        std::cout << "Price (0 for market order): ";
        std::cin >> price;

        type = (price > 0) ? "limit" : "market";

        try {
            DeribitTrader::OrderRequest order{
                .instrument_name = current_instrument_,
                .direction = direction,
                .amount = amount,
                .price = price,
                .type = type,
                .post_only = false,
                .reduce_only = false
            };

            std::string order_id = trader_.place_order(order);
            std::cout << "Order placed successfully. Order ID: " << order_id << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Order placement failed: " << e.what() << std::endl;
        }

        wait_for_user();
    }

    void cancel_order() {
        clear_screen();
        std::string order_id;
        std::cout << "Enter Order ID to cancel: ";
        std::cin >> order_id;

        try {
            bool cancelled = trader_.cancel_order(order_id);
            if (cancelled) {
                std::cout << "Order cancelled successfully." << std::endl;
            } else {
                std::cout << "Failed to cancel order." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Order cancellation failed: " << e.what() << std::endl;
        }

        wait_for_user();
    }

    void modify_order() {
        clear_screen();
        std::string order_id;
        double new_amount, new_price;

        std::cout << "Enter Order ID to modify: ";
        std::cin >> order_id;

        std::cout << "New Amount: ";
        std::cin >> new_amount;

        std::cout << "New Price: ";
        std::cin >> new_price;

        try {
            bool modified = trader_.modify_order(order_id, new_amount, new_price);
            if (modified) {
                std::cout << "Order modified successfully." << std::endl;
            } else {
                std::cout << "Failed to modify order." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Order modification failed: " << e.what() << std::endl;
        }

        wait_for_user();
    }

    void get_open_orders() {
        clear_screen();
        try {
            auto orders = trader_.get_open_orders(current_instrument_);
            
            if (orders.empty()) {
                std::cout << "No open orders." << std::endl;
            } else {
                std::cout << "Open Orders:\n";
                std::cout << std::left 
                          << std::setw(20) << "Order ID"
                          << std::setw(15) << "Direction"
                          << std::setw(15) << "Amount"
                          << std::setw(15) << "Price"
                          << std::setw(15) << "Type"
                          << std::setw(15) << "State" << std::endl;
                
                for (const auto& order : orders) {
                    std::cout << std::left 
                              << std::setw(20) << order.order_id
                              << std::setw(15) << order.direction
                              << std::setw(15) << order.amount
                              << std::setw(15) << order.price
                              << std::setw(15) << order.order_type
                              << std::setw(15) << order.order_state << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to fetch open orders: " << e.what() << std::endl;
        }

        wait_for_user();
    }

    void view_order_history() {

        clear_screen();
        std::cout << "TODO : Order History" << std::endl;
        wait_for_user();
    }

    void change_instrument() {
        clear_screen();
        std::cout << "Enter new instrument name (e.g., BTC-PERPETUAL, ETH-PERPETUAL): ";
        std::cin >> current_instrument_;
        
        try {
            trader_.get_orderbook(current_instrument_);
            std::cout << "Switched to instrument: " << current_instrument_ << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Invalid instrument. Reverting to previous instrument." << std::endl;
            current_instrument_ = "BTC-PERPETUAL";
        }

        wait_for_user();
    }

    void real_time_market_analysis() {
        clear_screen();
        display_market_overview(true);
        wait_for_user();
    }

    void update_market_data() {
        while (running_) {
            try {

                json orderbook = trader_.get_orderbook(current_instrument_);
                
                if (orderbook.contains("result")) {
                    const auto& result = orderbook["result"];
                    
                    market_data_.best_bid = result["bids"][0][0].get<double>();
                    market_data_.best_ask = result["asks"][0][0].get<double>();
                    market_data_.last_updated = std::chrono::steady_clock::now();
                }
            } catch (const std::exception& e) {
                // Silently handle errors in background thread
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
            
            // Update every 5 seconds
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    void display_market_overview(bool detailed = false) {
        std::cout << "Market Overview for " << current_instrument_ << ":\n";
        
        // Check if market data is fresh
        auto now = std::chrono::steady_clock::now();
        auto age = std::chrono::duration_cast<std::chrono::seconds>(
            now - market_data_.last_updated).count();
        
        if (age > 10) {
            std::cout << "Market data is stale (last updated " << age << " seconds ago)\n";
            return;
        }

        std::cout << "Best Bid: $" << market_data_.best_bid << std::endl;
        std::cout << "Best Ask: $" << market_data_.best_ask << std::endl;
        std::cout << "Spread: $" << (market_data_.best_ask - market_data_.best_bid) << std::endl;

        if (detailed) {
            double mid_price = (market_data_.best_bid + market_data_.best_ask) / 2;
            std::cout << "\nAdvanced Market Analysis:\n";
            std::cout << "Mid Price: $" << mid_price << std::endl;
            std::cout << "Potential Trade Signals:\n";
            
            if (market_data_.best_bid < mid_price * 0.99) {
                std::cout << "  - Potential Buying Opportunity\n";
            }
            
            if (market_data_.best_ask > mid_price * 1.01) {
                std::cout << "  - Potential Selling Opportunity\n";
            }
        }
    }

    // Utility methods
    void clear_screen() {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
    }

    void wait_for_user() {
        std::cout << "\nPress Enter to continue...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.get();
    }
};



int main() {
    try {

        DeribitTrader trader("Key", "Secret");

        TradingApp app(trader);
        app.run();


    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}