//
// Created by William Leonard Sumendap on 7/10/20.
//

#include "TradeEngine.h"
#include <pthread.h>

TradeEngine::TradeEngine(string conn) : C(conn)
{
}

string TradeEngine::createUser(string name, string password)
{
    pqxx::work W{C};
    json response;
    try
    {
        string query = "INSERT INTO ts.login (username, password) VALUES ('" +
                       name + "','" + password + "')";
        W.exec(query);
        W.commit();
        response = {
            {"createUserResponse", {}}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response.dump();
}

string TradeEngine::deleteBuyOrder(string username, long long orderId)
{
    //lazy deletion (?)
    pqxx::work W{C};
    json response;
    try
    {
        string query = "DELETE FROM ts.buy_orders WHERE order_id = '" +
                       to_string(orderId) + "' AND username = '" + username + "')";
        W.exec(query);
        W.commit();
        response = {
            {"deleteBuyOrderResponse", {}}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response.dump();
}

string TradeEngine::deleteSellOrder(string username, long long orderId)
{
    //lazy deletion (?)
    pqxx::work W{C};
    json response;
    try
    {
        string query = "DELETE FROM ts.sell_orders WHERE order_id = '" +
                       to_string(orderId) + "' AND username = '" + username + "')";
        W.exec(query);
        W.commit();
        response = {
            {"deleteSellOrderResponse", {}}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response.dump();
}

string TradeEngine::getBuyVolumes()
{
    pqxx::work W{C};
    json response;
    try
    {
        string query = "SELECT price, SUM(amount) FROM ts.buy_orders GROUP BY price ORDER BY price DESC";
        pqxx::result R{W.exec(query)};
        json entries;
        for (auto row : R)
        {
            int price = row[0].as<int>();
            int vol = row[1].as<int>();
            json entry = {
                {"price", price},
                {"volume", vol}
            };
            entries.push_back(entry);
        }
        response = {
            {"getBuyVolumesResponse", entries}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response.dump();
}

string TradeEngine::getSellVolumes()
{
    pqxx::work W{C};
    json response;
    try
    {
        string query = "SELECT price, SUM(amount) FROM ts.sell_orders GROUP BY price ORDER BY price ASC";
        pqxx::result R{W.exec(query)};
        json entries;
        for (auto row : R)
        {
            int price = row[0].as<int>();
            int vol = row[1].as<int>();
            json entry = {
                {"price", price},
                {"volume", vol}
            };
            entries.push_back(entry);
        }
        response = {
            {"getSellVolumesResponse", entries}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response.dump();
}

string TradeEngine::placeBuyOrder(string buyer, int price, int amt)
{
    json response;
    try
    {
        pqxx::work W{C};
        int currAmt = amt;
        ostringstream query;
        query << "SELECT * FROM ts.sell_orders WHERE price = LEAST((SELECT MIN(price) "
            << "FROM ts.sell_orders), " << price << ") ORDER BY order_id ASC";

        pqxx::result R{W.exec(query.str())};
        vector<pair<long long, int>> partiallyConvertedOrders;
        json trades;
        while (R.size() > 0 && currAmt > 0)
        {
            int currPrice = R[0][3].as<int>();
            for (auto row : R)
            {
                long long order_id = row[0].as<long long>();
                string seller = row[1].as<string>();
                int amt = row[2].as<int>();
                int sellPrice = row[3].as<int>();
                if (currAmt >= amt)
                {
                    json trade = {
                        {"price", sellPrice},
                        {"amount", amt},
                        {"buyer", buyer},
                        {"seller", seller}
                    };
                    trades.push_back(trade);
                    currAmt -= amt;
                }
                else
                {
                    json trade = {
                        {"price", sellPrice},
                        {"amount", currAmt},
                        {"buyer", buyer},
                        {"seller", seller}
                    };
                    partiallyConvertedOrders.push_back(make_pair(order_id, amt - currAmt));
                    trades.push_back(trade);
                    currAmt = 0;
                    break; // BREAK WHICH LOOP?
                }
            }
            // delete fully converted orders
            ostringstream deleteQuery;
            deleteQuery << "DELETE FROM ts.buy_orders WHERE price = " << currPrice;
            if (partiallyConvertedOrders.size() > 0)
            {
                deleteQuery << " AND order_id < " << partiallyConvertedOrders[0].first;
            }
            W.exec(deleteQuery.str());
            R = {W.exec(query.str())}; // CONFIRM HOW TO DO THIS
        }
        // update partially converted order (if any)
        for (int i = 0; i < partiallyConvertedOrders.size(); i++)
        {
            ostringstream updateQuery;
            int amountLeft = partiallyConvertedOrders[i].second;
            long long order_id = partiallyConvertedOrders[i].first;
            updateQuery << "UPDATE ts.sell_orders SET amount = " << amountLeft
                << " WHERE order_id = " << order_id;
            W.exec(updateQuery.str());
        }
        // insert newly created trades
        for (int i = 0; i < trades.size(); i++)
        {
            json &trade = trades[i];
            // CONFIRM THE SYNTAX
            int tradePrice = trade["price"].get<int>();
            int tradeAmount = trade["amount"].get<int>();
            string tradeBuyer = "'" + trade["buyer"].get<string>() + "'";
            string tradeSeller = "'" + trade["seller"].get<string>() + "'";

            ostringstream insertTradeQuery;
            insertTradeQuery << "INSERT INTO ts.trades (amount, price, buyer, seller)" 
                << " VALUES (" << tradeAmount << ", "
                << tradePrice << ", " << tradeBuyer << ", " << tradeSeller << ")";
            W.exec(insertTradeQuery.str());
        }
        // if some amount left untraded, insert to buy_orders
        if (currAmt > 0)
        {
            ostringstream insertOrderQuery;
            insertOrderQuery << "INSERT INTO ts.buy_orders (username, amount, price) " 
                << " VALUES (" << buyer << ", " 
                << currAmt << ", " << price << ", true)";
            W.exec(insertOrderQuery.str());
        }
        W.commit();
        response = {
            {"placeBuyOrderResponse", trades}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response.dump();
}

string TradeEngine::placeSellOrder(string seller, int price, int amt)
{
    json response;
    try
    {
        pqxx::work W{C};
        int currAmt = amt;
        ostringstream query;
        query << "SELECT * FROM ts.buy_orders WHERE price = GREATEST((SELECT MAX(price) "
            << "FROM ts.buy_orders), " << price << ") ORDER BY order_id ASC";

        pqxx::result R{W.exec(query.str())};
        vector<pair<long long, int>> partiallyConvertedOrders;
        json trades;
        while (R.size() > 0 && currAmt > 0)
        {
            int currPrice = R[0][3].as<int>();
            for (auto row : R)
            {
                long long order_id = row[0].as<long long>();
                string buyer = row[1].as<string>();
                int amt = row[2].as<int>();
                int buyPrice = row[3].as<int>();
                if (currAmt >= amt)
                {
                    json trade = {
                        {"price", buyPrice},
                        {"amount", amt},
                        {"buyer", buyer},
                        {"seller", seller}
                    };
                    trades.push_back(trade);
                    currAmt -= amt;
                }
                else
                {
                    json trade = {
                        {"price", buyPrice},
                        {"amount", currAmt},
                        {"buyer", buyer},
                        {"seller", seller}
                    };
                    partiallyConvertedOrders.push_back(make_pair(order_id, amt - currAmt));
                    trades.push_back(trade);
                    currAmt = 0;
                    break; // BREAK WHICH LOOP?
                }
            }
            // delete fully converted orders
            ostringstream deleteQuery;
            deleteQuery << "DELETE FROM ts.buy_orders WHERE price = " << currPrice;
            if (partiallyConvertedOrders.size() > 0)
            {
                deleteQuery << " AND order_id < " << partiallyConvertedOrders[0].first;
            }
            W.exec(deleteQuery.str());
            R = {W.exec(query.str())}; // CONFIRM HOW TO DO THIS
        }
        // update partially converted order (if any)
        for (int i = 0; i < partiallyConvertedOrders.size(); i++)
        {
            ostringstream updateQuery;
            int amountLeft = partiallyConvertedOrders[i].second;
            long long order_id = partiallyConvertedOrders[i].first;
            updateQuery << "UPDATE ts.buy_orders SET amount = " << amountLeft
                << " WHERE order_id = " << order_id;
            W.exec(updateQuery.str());
        }
        // insert newly created trades
        for (int i = 0; i < trades.size(); i++)
        {
            json &trade = trades[i];
            // CONFIRM THE SYNTAX
            int tradePrice = trade["price"].get<int>();
            int tradeAmount = trade["amount"].get<int>();
            string tradeBuyer = "'" + trade["buyer"].get<string>() + "'";
            string tradeSeller = "'" + trade["seller"].get<string>() + "'";

            ostringstream insertTradeQuery;
            insertTradeQuery << "INSERT INTO ts.trades (amount, price, buyer, seller)" 
                << " VALUES (" << tradeAmount << ", "
                << tradePrice << ", " << tradeBuyer << ", " << tradeSeller << ")";
            W.exec(insertTradeQuery.str());
        }
        // if some amount left untraded, insert to buy_orders
        if (currAmt > 0)
        {
            ostringstream insertOrderQuery;
            insertOrderQuery << "INSERT INTO ts.sell_orders (username, amount, price)" 
                << " VALUES (" << seller << ", "
                << currAmt << ", " << price << ", true)";
            W.exec(insertOrderQuery.str());
        }
        W.commit();
        response = {
            {"placeBuyOrderResponse", trades}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response.dump();
}

string TradeEngine::getPendingBuyOrders(string username)
{
    pqxx::work W{C};
    json response;
    try
    {
        string query = "SELECT * FROM ts.buy_orders WHERE username = '" +
                    username + "'";
        pqxx::result R{W.exec(query)};
        json entries;
        for (auto row : R)
        {
            long long order_id = row[0].as<long long>();
            string username = row[1].as<string>();
            int amt = row[2].as<int>();
            int price = row[3].as<int>();
            json entry = {
                {"order_id", order_id},
                {"price", price},
                {"amount", amt},
                {"price", price}
            };
            entries.push_back(entry);
        }
        response = {
            {"getPendingBuyOrdersResponse", entries}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response.dump();
}

string TradeEngine::getPendingSellOrders(string username)
{
    pqxx::work W{C};
    json response;
    try
    {
        string query = "SELECT * FROM ts.sell_orders WHERE username = '" +
                    username + "'";
        pqxx::result R{W.exec(query)};
        json entries;
        for (auto row : R)
        {
            long long order_id = row[0].as<long long>();
            string username = row[1].as<string>();
            int amt = row[2].as<int>();
            int price = row[3].as<int>();
            json entry = {
                {"order_id", order_id},
                {"price", price},
                {"amount", amt},
                {"price", price}
            };
            entries.push_back(entry);
        }
        response = {
            {"getPendingSellOrdersResponse", entries}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response.dump();
}

string TradeEngine::getBuyTrades(string username)
{
    pqxx::work W{C};
    json response;
    try
    {
        string query = "SELECT * FROM ts.trades WHERE buyer = '" +
                    username + "'";
        pqxx::result R{W.exec(query)};
        json entries;
        for (auto row : R)
        {
            long long trade_id = row[0].as<long long>();
            int amt = row[1].as<int>();
            int price = row[2].as<int>();
            string buyer = row[3].as<string>();
            string seller = row[4].as<string>();
            json entry = {
                {"trade_id", trade_id},
                {"price", price},
                {"amount", amt},
                {"buyer", buyer},
                {"seller", seller}
            };
            entries.push_back(entry);
        }
        response = {
            {"getBuyTradesResponse", entries}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response.dump();
}

string TradeEngine::getSellTrades(string username)
{
    pqxx::work W{C};
    json response;
    try
    {
        string query = "SELECT * FROM ts.trades WHERE seller = '" +
                    username + "'";
        pqxx::result R{W.exec(query)};
        json entries;
        for (auto row : R)
        {
            long long trade_id = row[0].as<long long>();
            int amt = row[1].as<int>();
            int price = row[2].as<int>();
            string buyer = row[3].as<string>();
            string seller = row[4].as<string>();
            json entry = {
                {"trade_id", trade_id},
                {"price", price},
                {"amount", amt},
                {"buyer", buyer},
                {"seller", seller}
            };
            entries.push_back(entry);
        }
        response = {
            {"getSellTradesResponse", entries}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response.dump();
}
