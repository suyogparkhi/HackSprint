#pragma once

#include <deque>
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <sstream>
#include "deribit_trader.hpp"

class TradingAgent {
public:
    enum class RiskLevel {
        CONSERVATIVE,
        MODERATE,
        AGGRESSIVE
    };

    enum class Strategy {
        MOMENTUM,
        MEAN_REVERSION,
        BREAKOUT
    };

    enum class MarketValueCondition {
        EQUAL_TO,
        LESS_THAN,
        GREATER_THAN,
        LESS_THAN_OR_EQUAL,
        GREATER_THAN_OR_EQUAL
    };

    struct Position {
        std::string order_id;
        std::string direction;  // "buy" or "sell"
        double entry_price;
        double amount;
        double current_pnl;    // Current profit/loss
        double highest_pnl;    // Highest profit reached
        double lowest_pnl;     // Lowest profit (max loss) reached
        std::chrono::system_clock::time_point entry_time;
    };

    struct TradingParams {
        double position_size;           // Size of each position
        double stop_loss;              // Stop loss percentage
        double take_profit;            // Take profit percentage
        double trailing_stop;          // Trailing stop percentage
        int lookback_period;           // Period for technical indicators
        double threshold;              // Entry/exit threshold
        int min_trade_interval;        // Minimum seconds between trades
        double max_position_size;      // Maximum total position size
        int max_open_positions;        // Maximum number of open positions
        double profit_target_daily;    // Daily profit target
        double max_loss_daily;         // Maximum daily loss limit
    };

    struct MandatoryOrderParams {
        double target_value;           // The market value to trigger the order
        MarketValueCondition condition; // Condition for order execution
        std::string direction;         // "buy" or "sell"
        double amount;                 // Order amount
        bool is_market_order;          // Whether to use market or limit order
        double limit_price;            // Limit price (if not a market order)
    };

    TradingAgent(DeribitTrader& trader, 
                 const std::string& instrument = "BTC-PERPETUAL",
                 RiskLevel risk = RiskLevel::CONSERVATIVE, 
                 Strategy strategy = Strategy::MOMENTUM);



    // Core trading functions
    void start();
    void stop();
    void updatePrice(double current_price, double bid_price, double ask_price);
    void processSignal();
    
    // Position management
    void checkPositions();
    std::vector<Position> getOpenPositions() const;
    double getCurrentPnL() const;
    double getDailyPnL() const;
    
    // Configuration
    void setRiskLevel(RiskLevel risk);
    void setStrategy(Strategy strategy);
    void setTradingParams(const TradingParams& params);
    
    // Status and metrics
    bool isRunning() const { return running; }
    double getTotalProfit() const { return total_profit; }
    int getTotalTrades() const { return total_trades; }
    double getWinRate() const;
    std::string getStrategyStatus() const;

    // Market analysis
    bool isVolatilityHigh() const;
    bool isMarketTrending() const;
    bool shouldContinueTrading() const;
    double adjustPositionSize(double base_size) const;

    void setMandatoryOrder(const MandatoryOrderParams& params);
    void clearMandatoryOrder();

private:

    MandatoryOrderParams mandatory_order_params;
    bool mandatory_order_configured = false;
    bool mandatory_order_triggered = false;

    // New private methods for initial order placement and mandatory order logic
    double determineOptimalOrderSize();
    std::string determineInitialOrderDirection();
    bool checkMandatoryOrderCondition(double current_price) const;
    void executeMandatoryOrder();


    // Core components
    DeribitTrader& trader;
    std::string current_instrument;
    RiskLevel risk_level;
    Strategy current_strategy;
    TradingParams params;
    bool running;

    // Market data
    std::deque<double> price_history;
    double current_price;
    double current_bid;
    double current_ask;
    std::chrono::system_clock::time_point last_trade_time;

    // Position tracking
    std::vector<Position> open_positions;
    std::map<std::string, Position> position_history;  // order_id -> Position
    
    // Performance metrics
    double total_profit;
    double daily_profit;
    int total_trades;
    int winning_trades;
    double highest_profit;
    double biggest_loss;
    std::chrono::system_clock::time_point trading_start_time;
    std::chrono::system_clock::time_point daily_reset_time;

    // Risk parameters
    const std::map<RiskLevel, TradingParams> risk_params = {
        {RiskLevel::CONSERVATIVE, {
            .position_size = 0.01,     // 1% of account
            .stop_loss = 0.02,         // 2% stop loss
            .take_profit = 0.04,       // 4% take profit
            .trailing_stop = 0.01,     // 1% trailing stop
            .lookback_period = 20,
            .threshold = 1.5,
            .min_trade_interval = 300,  // 5 minutes
            .max_position_size = 0.05,  // 5% max total position
            .max_open_positions = 3,
            .profit_target_daily = 0.05, // 5% daily target
            .max_loss_daily = 0.03      // 3% max daily loss
        }},
        {RiskLevel::MODERATE, {
            .position_size = 0.02,     // 2% of account
            .stop_loss = 0.03,         // 3% stop loss
            .take_profit = 0.06,       // 6% take profit
            .trailing_stop = 0.015,    // 1.5% trailing stop
            .lookback_period = 14,
            .threshold = 2.0,
            .min_trade_interval = 180,  // 3 minutes
            .max_position_size = 0.1,   // 10% max total position
            .max_open_positions = 5,
            .profit_target_daily = 0.08, // 8% daily target
            .max_loss_daily = 0.05      // 5% max daily loss
        }},
        {RiskLevel::AGGRESSIVE, {
            .position_size = 0.03,     // 3% of account
            .stop_loss = 0.05,         // 5% stop loss
            .take_profit = 0.1,        // 10% take profit
            .trailing_stop = 0.02,     // 2% trailing stop
            .lookback_period = 10,
            .threshold = 2.5,
            .min_trade_interval = 60,   // 1 minute
            .max_position_size = 0.2,   // 20% max total position
            .max_open_positions = 8,
            .profit_target_daily = 0.15, // 15% daily target
            .max_loss_daily = 0.1       // 10% max daily loss
        }}
    };

    // Strategy implementations
    bool checkMomentumSignal();
    bool checkMeanReversionSignal();
    bool checkBreakoutSignal();
    
    // Position management
    void enterPosition(const std::string& direction);
    void exitPosition(const std::string& order_id);
    void updatePositionPnL();
    void manageStopLoss();
    void manageTrailingStop();
    void manageTakeProfit();
    bool checkTradeTimeRestrictions();
    bool checkRiskLimits();
    void updateDailyMetrics();
    
    // Technical indicators
    double calculateRSI(const std::vector<double>& prices, int period) const;
    double calculateSMA(const std::vector<double>& prices, int period) const;
    double calculateVolatility(const std::vector<double>& prices, int period) const;
    bool detectBreakout(const std::vector<double>& prices, double current_price) const;

    // Utilities
    void resetDailyMetrics();
    void logTrade(const std::string& order_id, const std::string& action, double price);
};