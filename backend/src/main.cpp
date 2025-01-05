#include "deribit_trader.hpp"
#include "trading_agent.hpp"
#include <iostream>
#include <iomanip>
#include <csignal>
#include <thread>
#include <atomic>
#include <fstream>

std::atomic<bool> g_running(true);

void signal_handler(int signal) {
    g_running = false;
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
}

class TradingApp {
private:

    std::ofstream log_file_;

    void log_message(const std::string& message) {
        if (log_file_.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            log_file_ << std::put_time(std::localtime(&in_time_t), "[%Y-%m-%d %H:%M:%S] ") 
                      << message << std::endl;
        }
    }

    void initialize_logging() {
        // Create a log file with timestamp
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream filename;
        filename << "trading_app_" 
                 << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S") 
                 << ".log";
        
        log_file_.open(filename.str(), std::ios::app);
        
        if (!log_file_.is_open()) {
            std::cerr << "Failed to open log file: " << filename.str() << std::endl;
        } else {
            log_message("Trading application started");
        }
    }

    DeribitTrader& trader_;
    TradingAgent agent_;
    bool running_ = true;
    std::string current_instrument_ = "BTC-PERPETUAL";

    struct MarketSnapshot {
        double best_bid = 0.0;
        double best_ask = 0.0;
        double last_price = 0.0;
        std::chrono::steady_clock::time_point last_updated;
    };
    MarketSnapshot market_data_;

public:
    TradingApp(DeribitTrader& trader) 
        : trader_(trader)
        , agent_(trader, "BTC-PERPETUAL", TradingAgent::RiskLevel::CONSERVATIVE) {

        initialize_logging();
    }

    void run() {
        // Start market data thread
        std::thread market_data_thread(&TradingApp::update_market_data, this);
        market_data_thread.detach();

        while (running_ && g_running) {
            display_main_menu();
        }
    }

private:
    void display_main_menu() {
        clear_screen();
        display_market_overview();
        display_trading_status();

        std::cout << "\n===== Deribit Trading Terminal =====\n";
        std::cout << "Current Instrument: " << current_instrument_ << "\n\n";
        std::cout << "1. Manual Trading\n";
        std::cout << "2. Configure Automated Trading\n";
        std::cout << "3. Start Automated Trading\n";
        std::cout << "4. Stop Automated Trading\n";
        std::cout << "5. View Positions & Performance\n";
        std::cout << "6. Change Instrument\n";
        std::cout << "7. Risk Management Settings\n";
        std::cout << "8. Exit\n";
        std::cout << "Enter your choice: ";

        int choice;
        std::cin >> choice;

        switch (choice) {
            case 1: manual_trading_menu(); break;
            case 2: configure_automated_trading(); break;
            case 3: start_automated_trading(); break;
            case 4: stop_automated_trading(); break;
            case 5: view_positions_and_performance(); break;
            case 6: change_instrument(); break;
            case 7: risk_management_settings(); break;
            case 8: running_ = false; break;
            default: 
                std::cout << "Invalid choice. Press Enter to continue...";
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cin.get();
        }
    }

    void manual_trading_menu() {
        clear_screen();
        std::cout << "Manual Trading Menu\n\n";
        std::cout << "1. Place Order\n";
        std::cout << "2. Cancel Order\n";
        std::cout << "3. Modify Order\n";
        std::cout << "4. View Open Orders\n";
        std::cout << "5. Back to Main Menu\n";
        std::cout << "Choice: ";

        int choice;
        std::cin >> choice;

        switch (choice) {
            case 1: place_manual_order(); break;
            case 2: cancel_order(); break;
            case 3: modify_order(); break;
            case 4: get_open_orders(); break;
            case 5: return;
        }
    }

    void place_manual_order() {
        clear_screen();
        std::cout << "Place Order for " << current_instrument_ << "\n\n";
        
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

    void configure_automated_trading() {
        clear_screen();
        std::cout << "Configure Automated Trading\n\n";

        // Strategy Selection
        std::cout << "Select Strategy:\n";
        std::cout << "1. Momentum Trading\n";
        std::cout << "2. Mean Reversion\n";
        std::cout << "3. Breakout Trading\n";
        std::cout << "Choice: ";

        int strategy_choice;
        std::cin >> strategy_choice;

        TradingAgent::Strategy strategy;
        switch (strategy_choice) {
            case 1: strategy = TradingAgent::Strategy::MOMENTUM; break;
            case 2: strategy = TradingAgent::Strategy::MEAN_REVERSION; break;
            case 3: strategy = TradingAgent::Strategy::BREAKOUT; break;
            default: strategy = TradingAgent::Strategy::MOMENTUM;
        }

        // Risk Level Selection
        std::cout << "\nSelect Risk Level:\n";
        std::cout << "1. Conservative\n";
        std::cout << "2. Moderate\n";
        std::cout << "3. Aggressive\n";
        std::cout << "Choice: ";

        int risk_choice;
        std::cin >> risk_choice;

        TradingAgent::RiskLevel risk;
        switch (risk_choice) {
            case 1: risk = TradingAgent::RiskLevel::CONSERVATIVE; break;
            case 2: risk = TradingAgent::RiskLevel::MODERATE; break;
            case 3: risk = TradingAgent::RiskLevel::AGGRESSIVE; break;
            default: risk = TradingAgent::RiskLevel::CONSERVATIVE;
        }

        agent_.setStrategy(strategy);
        agent_.setRiskLevel(risk);

        std::cout << "\nTrading configuration updated successfully!" << std::endl;
        wait_for_user();
    }

    void start_automated_trading() {
        if (!agent_.isRunning()) {
            agent_.start();
            std::cout << "Automated trading started!" << std::endl;
        } else {
            std::cout << "Trading already running!" << std::endl;
        }
        wait_for_user();
    }

    void stop_automated_trading() {
        if (agent_.isRunning()) {
            agent_.stop();
            std::cout << "Automated trading stopped!" << std::endl;
        } else {
            std::cout << "Trading already stopped!" << std::endl;
        }
        wait_for_user();
    }

    void view_positions_and_performance() {
        clear_screen();
        std::cout << agent_.getStrategyStatus() << std::endl;
        wait_for_user();
    }

    void risk_management_settings() {
        clear_screen();
        std::cout << "Risk Management Settings\n\n";
        
        TradingAgent::TradingParams params = {
            .position_size = 0.0,
            .stop_loss = 0.0,
            .take_profit = 0.0,
            .trailing_stop = 0.0,
            .lookback_period = 0,
            .threshold = 0.0,
            .min_trade_interval = 0,
            .max_position_size = 0.0,
            .max_open_positions = 0,
            .profit_target_daily = 0.0,
            .max_loss_daily = 0.0
        };

        std::cout << "Enter Position Size (0.01-1.0): ";
        std::cin >> params.position_size;

        std::cout << "Enter Stop Loss (%): ";
        std::cin >> params.stop_loss;
        params.stop_loss /= 100.0;

        std::cout << "Enter Take Profit (%): ";
        std::cin >> params.take_profit;
        params.take_profit /= 100.0;

        std::cout << "Enter Daily Loss Limit (%): ";
        std::cin >> params.max_loss_daily;
        params.max_loss_daily /= 100.0;

        agent_.setTradingParams(params);
        std::cout << "\nRisk parameters updated successfully!" << std::endl;
        wait_for_user();
    }

    void update_market_data() {
        while (running_ && g_running) {
            try {
                json orderbook = trader_.get_orderbook(current_instrument_);
                
                if (orderbook.contains("result")) {
                    const auto& result = orderbook["result"];
                    market_data_.best_bid = result["bids"][0][0].get<double>();
                    market_data_.best_ask = result["asks"][0][0].get<double>();
                    market_data_.last_updated = std::chrono::steady_clock::now();
                    
                    // Update trading agent with new prices
                    if (agent_.isRunning()) {
                        double mid_price = (market_data_.best_bid + market_data_.best_ask) / 2;
                        agent_.updatePrice(mid_price, market_data_.best_bid, market_data_.best_ask);
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Market data update error: " << e.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
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

    void display_market_overview() {
        std::cout << "Market Overview for " << current_instrument_ << ":\n";
        
        auto now = std::chrono::steady_clock::now();
        auto age = std::chrono::duration_cast<std::chrono::seconds>(
            now - market_data_.last_updated).count();
        
        if (age > 10) {
            std::cout << "Market data is stale (last updated " << age << " seconds ago)\n";
            return;
        }

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Best Bid: $" << market_data_.best_bid << std::endl;
        std::cout << "Best Ask: $" << market_data_.best_ask << std::endl;
        std::cout << "Spread: $" << (market_data_.best_ask - market_data_.best_bid) << std::endl;
    }

    void display_trading_status() {
        if (agent_.isRunning()) {
            std::cout << "\nTrading Bot Status: ACTIVE\n";
            std::cout << "Current P&L: $" << agent_.getCurrentPnL() << std::endl;
            std::cout << "Daily P&L: $" << agent_.getDailyPnL() << std::endl;
            std::cout << "Open Positions: " << agent_.getOpenPositions().size() << std::endl;
        } else {
            std::cout << "\nTrading Bot Status: INACTIVE\n";
        }
    }

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
        // Add more comprehensive logging and error handling
        std::ofstream main_log("trading_main.log", std::ios::app);
        
        if (!main_log.is_open()) {
            std::cerr << "Could not open main log file!" << std::endl;
            return 1;
        }

        auto log_message = [&main_log](const std::string& message) {
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            main_log << std::put_time(std::localtime(&in_time_t), "[%Y-%m-%d %H:%M:%S] ") 
                     << message << std::endl;
        };

        log_message("=== Trading Application Started ===");

        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);

        std::cout << "=== Deribit Automated Trading System ===\n\n";
        
        std::string api_key = "IfKb1DKS";
        std::string api_secret = "OsySwvQZLZDNamcmit07SnXZvZmRVYk6AWL4zVRw-LE";

        if (api_key.empty() || api_secret.empty()) {
            log_message("API key or secret is empty");
            std::cerr << "API key and secret cannot be empty!" << std::endl;
            return 1;
        }

        try {
            DeribitTrader trader(api_key, api_secret);
            log_message("Trader initialized successfully");
            
            TradingApp app(trader);
            
            app.run();
        } catch (const std::exception& e) {
            log_message("Trader initialization failed: " + std::string(e.what()));
            std::cerr << "Trader initialization failed: " << e.what() << std::endl;
            return 1;
        }

        log_message("=== Trading Application Completed ===");
        main_log.close();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}