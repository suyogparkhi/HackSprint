#include "trading_agent.hpp"
#include <numeric>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <sstream>

TradingAgent::TradingAgent(DeribitTrader& trader_instance, 
                         const std::string& instrument,
                         RiskLevel risk, 
                         Strategy strategy)
    : trader(trader_instance)
    , current_instrument(instrument)
    , risk_level(risk)
    , current_strategy(strategy)
    , running(false)
    , total_profit(0.0)
    , daily_profit(0.0)
    , total_trades(0)
    , winning_trades(0)
    , highest_profit(0.0)
    , biggest_loss(0.0) {
    
    // Setup file logging
    try {
        // Create a log file with timestamp
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream filename;
        filename << "trading_log_" 
                 << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S") 
                 << ".log";
        
        auto file_logger = spdlog::basic_logger_mt("trading_logger", filename.str());
        
        spdlog::set_default_logger(file_logger);
        
        spdlog::set_level(spdlog::level::debug);

        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    }
    catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
    }
    
    params = risk_params.at(risk_level);
    trading_start_time = std::chrono::system_clock::now();
    resetDailyMetrics();
}

double TradingAgent::determineOptimalOrderSize() {
    try {
        if (price_history.size() < params.lookback_period) {
            spdlog::warn("Insufficient price history. Using default order size.");
            return params.position_size * 1000;  
        }

        // Calculate market volatility
        double volatility = calculateVolatility(
            std::vector<double>(price_history.begin(), price_history.end()), 
            params.lookback_period
        );

        // Calculate average price
        double avg_price = calculateSMA(
            std::vector<double>(price_history.begin(), price_history.end()), 
            params.lookback_period
        );

        // More volatile markets -> smaller position size
        double volatility_ratio = volatility / avg_price;
        
        // Base scaling factor
        double base_size = params.position_size * 1000;  

        // Higher volatility reduces position size
        if (volatility_ratio > 0.02) {  // High volatility threshold
            base_size *= std::max(0.5, 1.0 - (volatility_ratio / 0.02));
        }

        // Limit order size based on total account risk
        double max_risk_per_trade = params.max_loss_daily / 2;  // Use half of daily loss limit
        double estimated_trade_risk = std::abs(volatility * base_size);

        if (estimated_trade_risk > max_risk_per_trade) {
            base_size = (max_risk_per_trade / estimated_trade_risk) * base_size;
        }

        // Ensure minimum order size
        base_size = std::max(base_size, 1.0);

        spdlog::info("Optimal order size calculated: {}, Volatility: {}, Avg Price: {}", 
                     base_size, volatility, avg_price);

        return base_size;
    } 
    catch (const std::exception& e) {
        spdlog::error("Error determining optimal order size: {}", e.what());
        // Fallback to default position size
        return params.position_size * 1000;
    }
}

std::string TradingAgent::determineInitialOrderDirection() {
    try {
        // If insufficient price history, default to buy
        if (price_history.size() < params.lookback_period) {
            spdlog::warn("Insufficient price history. Defaulting to buy direction.");
            return "buy";
        }

        // Convert deque to vector for calculations
        std::vector<double> prices(price_history.begin(), price_history.end());

        // Calculate RSI to determine market sentiment
        double rsi = calculateRSI(prices, params.lookback_period);
        
        // Calculate moving average crossover
        std::vector<double> recent_prices(prices.end() - params.lookback_period, prices.end());
        double short_sma = calculateSMA(recent_prices, params.lookback_period);
        double long_sma = calculateSMA(prices, prices.size());

        // Determine direction based on multiple indicators
        if (rsi < 30 || short_sma > long_sma) {
            spdlog::info("Initial order direction: BUY (RSI: {}, SMA Crossover)", rsi);
            return "buy";
        } else if (rsi > 70 || short_sma < long_sma) {
            spdlog::info("Initial order direction: SELL (RSI: {}, SMA Crossover)", rsi);
            return "sell";
        }


        spdlog::info("No clear direction. Defaulting to BUY.");
        return "buy";
    } 
    catch (const std::exception& e) {
        spdlog::error("Error determining initial order direction: {}", e.what());
        return "buy"; 
    }
}



void TradingAgent::start() {
    running = true;
    spdlog::info("Starting automated trading with {} strategy", 
                 static_cast<int>(current_strategy));
    
    // Subscribe to market data
    trader.subscribe_orderbook(current_instrument);
    trader.subscribe_trades(current_instrument);

    // Wait a short time to gather initial market data
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Determine initial order parameters
    std::string direction = determineInitialOrderDirection();
    double order_size = determineOptimalOrderSize();

    // Prepare order request
    DeribitTrader::OrderRequest order{
        .instrument_name = current_instrument,
        .direction = direction,
        .amount = 100,
        .type = "market", 
    };

    try {
        // Place initial order
        std::string order_id = trader.place_order(order);
        
        if (!order_id.empty()) {
            // Record position
            Position pos{
                .order_id = order_id,
                .direction = direction,
                .entry_price = (direction == "buy") ? current_ask : current_bid,
                .amount = order_size,
                .current_pnl = 0.0,
                .highest_pnl = 0.0,
                .lowest_pnl = 0.0,
                .entry_time = std::chrono::system_clock::now()
            };
            
            open_positions.push_back(pos);
            position_history[order_id] = pos;
            last_trade_time = std::chrono::system_clock::now();
            
            spdlog::info("Initial position entered - Order ID: {}, Direction: {}, Amount: {}, Price: {}", 
                        order_id, direction, order_size, pos.entry_price);
        } else {
            spdlog::error("Failed to place initial order");
        }
    } 
    catch (const std::exception& e) {
        spdlog::error("Error placing initial order: {}", e.what());
    }
}

void TradingAgent::stop() {
    running = false;
    // Close all open positions
    for (const auto& position : open_positions) {
        exitPosition(position.order_id);
    }
    spdlog::info("Automated trading stopped. Final profit: {}", total_profit);
}



void TradingAgent::updatePrice(double price, double bid_price, double ask_price) {
    current_price = price;
    current_bid = bid_price;
    current_ask = ask_price;
    
    price_history.push_back(price);
    if (price_history.size() > params.lookback_period) {
        price_history.pop_front();
    }
    
    if (running) {
        updatePositionPnL();  // Update P&L for existing positions
        processSignal();      // Check for new trading signals
    }

    // Log current state
    spdlog::info("Price Update - Bid: {}, Ask: {}, Mid: {}", bid_price, ask_price, price);
    if (!price_history.empty()) {
        spdlog::info("Price History Size: {}", price_history.size());
    }
}

void TradingAgent::processSignal() {
    if (!checkTradeTimeRestrictions() || !checkRiskLimits()) {
        return;
    }

    if (price_history.size() < params.lookback_period) {
        spdlog::info("Insufficient price history. Current size: {}", price_history.size());
        return;
    }

    bool should_enter = false;
    std::string direction;

    // Convert deque to vector for calculations
    std::vector<double> prices(price_history.begin(), price_history.end());

    try {
        switch (current_strategy) {
            case Strategy::MOMENTUM: {
                double rsi = calculateRSI(prices, params.lookback_period);
                spdlog::info("Current RSI: {}", rsi);
                
                if (rsi > 70) {
                    should_enter = true;
                    direction = "sell";  // Overbought - sell signal
                } else if (rsi < 30) {
                    should_enter = true;
                    direction = "buy";   // Oversold - buy signal
                }
                break;
            }
            
            case Strategy::MEAN_REVERSION: {
                double sma = calculateSMA(prices, params.lookback_period);
                double deviation = (current_price - sma) / sma;
                spdlog::info("Price deviation from SMA: {}%", deviation * 100);
                
                if (std::abs(deviation) > params.threshold) {
                    should_enter = true;
                    direction = (deviation > 0) ? "sell" : "buy";
                }
                break;
            }
            
            case Strategy::BREAKOUT: {
                bool breakout = detectBreakout(prices, current_price);
                if (breakout) {
                    should_enter = true;
                    double prev_price = prices[prices.size() - 2];
                    direction = (current_price > prev_price) ? "buy" : "sell";
                }
                break;
            }
        }

        // Check if we can enter a new position
        if (should_enter && open_positions.empty()) {
            spdlog::info("Signal detected: {} signal for {}", direction, current_instrument);
            enterPosition(direction);
        }

    } catch (const std::exception& e) {
        spdlog::error("Error in processSignal: {}", e.what());
    }
}

void TradingAgent::enterPosition(const std::string& direction) {
    try {
        // Calculate position size based on risk parameters
        double position_size = params.position_size;
        position_size = adjustPositionSize(position_size);  // Adjust based on volatility

        // More robust price determination
        double order_price;
        if (direction == "buy") {
            // Add a small buffer to ensure order is placed
            order_price = current_ask * 1.005;  // 0.5% above ask price
        } else {
            // Add a small buffer to ensure order is placed
            order_price = current_bid * 0.995;  // 0.5% below bid price
        }

        // Create order request with more robust parameters
        DeribitTrader::OrderRequest order{
            .instrument_name = current_instrument,
            .direction = direction,
            .amount = std::max(1.0, position_size * 1000),  // Ensure minimum order size
            .price = order_price,
            .type = "limit",
            .post_only = false,
            .reduce_only = false
        };
        
        spdlog::info("Placing {} order: Amount = {}, Price = {}", 
                     direction, order.amount, order.price);

        // Place order with multiple retry mechanism
        std::string order_id;
        int max_retries = 3;
        for (int retry = 0; retry < max_retries; ++retry) {
            try {
                order_id = trader.place_order(order);
                if (!order_id.empty()) {
                    break;  // Successful order placement
                }
            } catch (const std::exception& e) {
                spdlog::error("Order placement attempt {} failed: {}", retry + 1, e.what());
                std::this_thread::sleep_for(std::chrono::seconds(1));  // Wait before retry
            }
        }
        
        if (order_id.empty()) {
            spdlog::error("Failed to place order after {} attempts", max_retries);
            return;
        }
        
        // Record position
        Position pos{
            .order_id = order_id,
            .direction = direction,
            .entry_price = order_price,
            .amount = order.amount,
            .current_pnl = 0.0,
            .highest_pnl = 0.0,
            .lowest_pnl = 0.0,
            .entry_time = std::chrono::system_clock::now()
        };
        
        open_positions.push_back(pos);
        position_history[order_id] = pos;
        last_trade_time = std::chrono::system_clock::now();
        
        spdlog::info("Position entered - Order ID: {}, Direction: {}, Price: {}", 
                    order_id, direction, pos.entry_price);
    } catch (const std::exception& e) {
        spdlog::error("Comprehensive position entry failed: {}", e.what());
    }
}

void TradingAgent::exitPosition(const std::string& order_id) {
    try {
        // Find the position
        auto it = std::find_if(open_positions.begin(), open_positions.end(),
            [&](const Position& pos) { return pos.order_id == order_id; });
            
        if (it == open_positions.end()) {
            return;
        }

        // Create exit order
        DeribitTrader::OrderRequest order{
            .instrument_name = current_instrument,
            .direction = (it->direction == "buy") ? "sell" : "buy",
            .amount = it->amount,
            .price = (it->direction == "buy") ? current_bid : current_ask,
            .type = "market",
            .post_only = false,
            .reduce_only = true
        };
        
        std::string exit_order_id = trader.place_order(order);
        
        if (!exit_order_id.empty()) {
            // Update metrics
            double exit_price = (it->direction == "buy") ? current_bid : current_ask;
            double pnl = it->current_pnl;
            
            total_profit += pnl;
            daily_profit += pnl;
            total_trades++;
            
            if (pnl > 0) {
                winning_trades++;
            }
            
            highest_profit = std::max(highest_profit, pnl);
            biggest_loss = std::min(biggest_loss, pnl);
            
            // Log the trade
            spdlog::info("Exited position {} with PnL: {}", order_id, pnl);
            
            // Remove from open positions
            open_positions.erase(it);
        }
    } catch (const std::exception& e) {
        spdlog::error("Failed to exit position: {}", e.what());
    }
}

void TradingAgent::updatePositionPnL() {
    for (auto& position : open_positions) {
        // Calculate current P&L
        double current_market_price = (position.direction == "buy") ? current_bid : current_ask;
        double price_diff = (position.direction == "buy") ? 
                          (current_market_price - position.entry_price) : 
                          (position.entry_price - current_market_price);
                          
        position.current_pnl = price_diff * position.amount;
        position.highest_pnl = std::max(position.highest_pnl, position.current_pnl);
        position.lowest_pnl = std::min(position.lowest_pnl, position.current_pnl);
        
        // Log P&L updates
        spdlog::info("Position P&L Update - Order ID: {}, Current P&L: {}, Highest: {}, Lowest: {}", 
                     position.order_id, position.current_pnl, position.highest_pnl, position.lowest_pnl);
        
        // Check stop loss and take profit
        double pnl_percentage = price_diff / position.entry_price;
        
        if (pnl_percentage <= -params.stop_loss || 
            pnl_percentage >= params.take_profit) {
            spdlog::info("SL/TP triggered for order {}: P&L = {}%", 
                        position.order_id, pnl_percentage * 100);
            exitPosition(position.order_id);
        }
    }
}

bool TradingAgent::checkMomentumSignal() {
    if (price_history.size() < params.lookback_period) {
        return false;
    }

    auto prices = std::vector<double>(price_history.begin(), price_history.end());
    double rsi = calculateRSI(prices, params.lookback_period);
    
    return (rsi > 70 || rsi < 30);
}

bool TradingAgent::checkMeanReversionSignal() {
    if (price_history.size() < params.lookback_period) {
        return false;
    }

    auto prices = std::vector<double>(price_history.begin(), price_history.end());
    double sma = calculateSMA(prices, params.lookback_period);
    double deviation = (current_price - sma) / sma;
    
    return std::abs(deviation) > params.threshold;
}

bool TradingAgent::checkBreakoutSignal() {
    if (price_history.size() < params.lookback_period) {
        return false;
    }

    auto prices = std::vector<double>(price_history.begin(), price_history.end());
    return detectBreakout(prices, current_price);
}

double TradingAgent::calculateRSI(const std::vector<double>& prices, int period) const {
    if (prices.size() < period + 1) return 50.0;

    std::vector<double> gains, losses;
    for (size_t i = 1; i < prices.size(); i++) {
        double diff = prices[i] - prices[i-1];
        gains.push_back(diff > 0 ? diff : 0);
        losses.push_back(diff < 0 ? -diff : 0);
    }
    
    double avg_gain = std::accumulate(gains.end() - period, gains.end(), 0.0) / period;
    double avg_loss = std::accumulate(losses.end() - period, losses.end(), 0.0) / period;
    
    return 100.0 - (100.0 / (1 + avg_gain / std::max(avg_loss, 0.0001)));
}

double TradingAgent::calculateSMA(const std::vector<double>& prices, int period) const {
    if (prices.size() < period) return prices.back();
    return std::accumulate(prices.end() - period, prices.end(), 0.0) / period;
}

double TradingAgent::calculateVolatility(const std::vector<double>& prices, int period) const {
    if (prices.size() < period) return 0.0;
    
    double mean = calculateSMA(prices, period);
    double sq_sum = std::inner_product(prices.end() - period, prices.end(), 
                                     prices.end() - period, 0.0,
                                     std::plus<>(),
                                     [mean](double x, double y) {
                                         return (x - mean) * (x - mean);
                                     });
                                     
    return std::sqrt(sq_sum / period);
}

bool TradingAgent::detectBreakout(const std::vector<double>& prices, double current_price) const {
    double max_price = *std::max_element(prices.begin(), prices.end());
    double min_price = *std::min_element(prices.begin(), prices.end());
    double range = max_price - min_price;
    
    return (current_price > max_price + range * 0.02) || 
           (current_price < min_price - range * 0.02);
}

void TradingAgent::checkPositions() {
    manageStopLoss();
    manageTrailingStop();
    manageTakeProfit();
}

void TradingAgent::manageStopLoss() {
    for (const auto& position : open_positions) {
        double pnl_percentage = position.current_pnl / 
                              (position.entry_price * position.amount);
                              
        if (pnl_percentage <= -params.stop_loss) {
            exitPosition(position.order_id);
        }
    }
}

void TradingAgent::manageTrailingStop() {
    for (const auto& position : open_positions) {
        if (position.highest_pnl > 0) {
            double drawdown = (position.highest_pnl - position.current_pnl) / 
                            position.highest_pnl;
                            
            if (drawdown > params.trailing_stop) {
                exitPosition(position.order_id);
            }
        }
    }
}

void TradingAgent::manageTakeProfit() {
    for (const auto& position : open_positions) {
        double pnl_percentage = position.current_pnl / 
                              (position.entry_price * position.amount);
                              
        if (pnl_percentage >= params.take_profit) {
            exitPosition(position.order_id);
        }
    }
}

bool TradingAgent::checkTradeTimeRestrictions() {
    auto now = std::chrono::system_clock::now();
    auto time_since_last_trade = 
        std::chrono::duration_cast<std::chrono::seconds>(now - last_trade_time).count();
        
    return time_since_last_trade >= params.min_trade_interval;
}

bool TradingAgent::checkRiskLimits() {
    // Check daily loss limit
    if (daily_profit < -params.max_loss_daily) {
        spdlog::warn("Daily loss limit reached. Stopping trading.");
        stop();
        return false;
    }
    
    // Check daily profit target
    if (daily_profit >= params.profit_target_daily) {
        spdlog::info("Daily profit target reached. Stopping trading.");
        stop();
        return false;
    }
    
    // Check maximum positions
    if (open_positions.size() >= params.max_open_positions) {
        return false;
    }
    
    // Check maximum position size
    double total_position_size = 0.0;
    for (const auto& pos : open_positions) {
        total_position_size += pos.amount;
    }
    
    return total_position_size < params.max_position_size;
}

void TradingAgent::resetDailyMetrics() {
    daily_profit = 0.0;
    daily_reset_time = std::chrono::system_clock::now();
}

void TradingAgent::updateDailyMetrics() {
    auto now = std::chrono::system_clock::now();
    auto hours_since_reset = 
        std::chrono::duration_cast<std::chrono::hours>(now - daily_reset_time).count();
        
    if (hours_since_reset >= 24) {
        resetDailyMetrics();
    }
}

double TradingAgent::getWinRate() const {
    return total_trades > 0 ? 
           (static_cast<double>(winning_trades) / total_trades) * 100.0 : 0.0;
}

std::string TradingAgent::getStrategyStatus() const {
    std::stringstream ss;
    ss << "Trading Strategy Status:\n"
       << "Strategy: " << static_cast<int>(current_strategy) << "\n"
       << "Risk Level: " << static_cast<int>(risk_level) << "\n"
       << "Total Profit: " << total_profit << "\n"
       << "Daily Profit: " << daily_profit << "\n"
       << "Win Rate: " << getWinRate() << "%\n"
       << "Total Trades: " << total_trades << "\n"
       << "Open Positions: " << open_positions.size() << "\n"
       << "Highest Profit: " << highest_profit << "\n"
       << "Biggest Loss: " << biggest_loss << "\n";

    if (!open_positions.empty()) {
        ss << "\nCurrent Positions:\n";
        for (const auto& pos : open_positions) {
            ss << "Order ID: " << pos.order_id << "\n"
               << "Direction: " << pos.direction << "\n"
               << "Entry Price: " << pos.entry_price << "\n"
               << "Amount: " << pos.amount << "\n"
               << "Current P&L: " << pos.current_pnl << "\n"
               << "-------------------\n";
        }
    }

    return ss.str();
}

void TradingAgent::setTradingParams(const TradingParams& new_params) {
    params = new_params;
    spdlog::info("Updated trading parameters");
}

std::vector<TradingAgent::Position> TradingAgent::getOpenPositions() const {
    return open_positions;
}

double TradingAgent::getCurrentPnL() const {
    double total_pnl = 0.0;
    for (const auto& pos : open_positions) {
        total_pnl += pos.current_pnl;
    }
    return total_pnl;
}

double TradingAgent::getDailyPnL() const {
    return daily_profit;
}

void TradingAgent::setRiskLevel(RiskLevel risk) {
    risk_level = risk;
    params = risk_params.at(risk_level);
    spdlog::info("Risk level updated to: {}", static_cast<int>(risk));
}

void TradingAgent::setStrategy(Strategy strategy) {
    current_strategy = strategy;
    price_history.clear();  // Reset price history for new strategy
    spdlog::info("Trading strategy updated to: {}", static_cast<int>(strategy));
}

void TradingAgent::logTrade(const std::string& order_id, 
                          const std::string& action, 
                          double price) {
    spdlog::info("{} trade: Order ID: {}, Price: {}", 
                 action, order_id, price);
}

// Add helper functions for monitoring price movements and volatility
bool TradingAgent::isVolatilityHigh() const {
    if (price_history.size() < params.lookback_period) {
        return false;
    }

    auto prices = std::vector<double>(price_history.begin(), price_history.end());
    double vol = calculateVolatility(prices, params.lookback_period);
    double avg_price = calculateSMA(prices, params.lookback_period);
    
    // Consider volatility high if it's more than 2% of average price
    return (vol / avg_price) > 0.02;
}

bool TradingAgent::isMarketTrending() const {
    if (price_history.size() < params.lookback_period * 2) {
        return false;
    }

    auto prices = std::vector<double>(price_history.begin(), price_history.end());
    double short_sma = calculateSMA(prices, params.lookback_period);
    double long_sma = calculateSMA(prices, params.lookback_period * 2);
    
    // Market is trending if short SMA is significantly different from long SMA
    return std::abs(short_sma - long_sma) / long_sma > 0.01;
}

// Additional safety checks
bool TradingAgent::shouldContinueTrading() const {
    // Check for rapid consecutive losses
    int recent_losses = 0;
    for (size_t i = position_history.size(); i > 0 && i > position_history.size() - 5; --i) {
        auto it = std::next(position_history.begin(), i - 1);
        if (it->second.current_pnl < 0) {
            recent_losses++;
        }
    }

    // Stop trading if we have 3 or more consecutive losses
    if (recent_losses >= 3) {
        spdlog::warn("Multiple consecutive losses detected. Consider pausing trading.");
        return false;
    }

    // Check for unusual market conditions
    if (isVolatilityHigh()) {
        spdlog::warn("High market volatility detected. Trading with caution.");
    }

    // Check daily metrics
    if (std::abs(daily_profit) > params.max_loss_daily) {
        spdlog::warn("Daily loss limit reached. Stopping trading.");
        return false;
    }

    return true;
}

// Helper method to adjust position sizes based on volatility
double TradingAgent::adjustPositionSize(double base_size) const {
    if (price_history.size() < params.lookback_period) {
        return base_size;
    }

    auto prices = std::vector<double>(price_history.begin(), price_history.end());
    double volatility = calculateVolatility(prices, params.lookback_period);
    double avg_price = calculateSMA(prices, params.lookback_period);
    double vol_ratio = volatility / avg_price;

    if (vol_ratio > 0.02) {
        return base_size * (0.02 / vol_ratio);
    }

    return base_size;
}