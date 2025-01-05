#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>
#include <thread>
#include <mutex>
#include <libwebsockets.h>
#include <nlohmann/json.hpp>
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

    DeribitTrader(const std::string& api_key, const std::string& api_secret);
    ~DeribitTrader();

    // Public methods
    struct lws_context* get_ws_context() { return ws_context_; }
    json get_instrument_details(const std::string& instrument_name);
    double round_to_contract_size(const std::string& instrument_name, double amount);
    double get_minimum_order_amount(const std::string& instrument_name);
    std::string place_order(const OrderRequest& request);
    bool cancel_order(const std::string& order_id);
    std::vector<OpenOrder> get_open_orders(const std::string& instrument_name = "");
    bool modify_order(const std::string& order_id, double new_amount, double new_price, 
                     const std::string& advanced = "");
    json get_orderbook(const std::string& instrument_name, int depth = 20);
    void subscribe_orderbook(const std::string& instrument_name);
    void subscribe_trades(const std::string& instrument_name);
    void on_ws_connect();
    void on_ws_message(const std::string& message);
    void on_ws_close();

private:
    // API credentials and connection details
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

    // Private methods
    static struct lws_protocols* get_protocols();
    void connect_websocket();
    void init_ssl();
    void init_websocket();
    void authenticate();
    json send_public_request(const std::string& endpoint, const json& params);
    json send_authenticated_request(const std::string& endpoint, const json& params);
    static int ws_callback(struct lws* wsi, enum lws_callback_reasons reason,
                          void* user, void* in, size_t len);
    void log_message_structure(const json& message);
    void handle_ws_authentication(const json& auth_response);
    void handle_ws_result(const json& result_response);
    void handle_ws_error(const json& error_response);
    void subscribe_default_channels();
    void send_ws_message(const std::string& message);
    static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp);
    void cleanup();
};