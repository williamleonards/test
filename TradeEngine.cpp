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
        string query = "INSERT INTO ts.login (username, password) VALUES (\"" +
                       name + "\",\"" + password + "\")";
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

// vector<Trade *> TradeEngine::placeBuyOrder(int issuerID, int price, int amt)
// {

//     pthread_mutex_lock(&usersLock);
//     pthread_mutex_lock(&buyLock);
//     pthread_mutex_lock(&sellLock);

//     User *user = users[issuerID];
//     vector<Trade *> ans;

//     if (user == NULL)
//     {
//         cout << "User ID not known" << endl;
//         return ans;
//     }

//     int remaining = amt;
//     // generate trades by matching the incoming order with pending sell orders
//     ans = generateTrades(true, price, issuerID, remaining);

//     if (remaining > 0)
//     { // put surplus amount on buy tree
//         putRemainingOrderOnTree(true, user, price, remaining);
//     }

//     pthread_mutex_unlock(&sellLock);
//     pthread_mutex_unlock(&buyLock);
//     pthread_mutex_unlock(&usersLock);

//     return ans;
// }

// vector<Trade *> TradeEngine::placeSellOrder(int issuerID, int price, int amt)
// {

//     pthread_mutex_lock(&usersLock);
//     pthread_mutex_lock(&buyLock);
//     pthread_mutex_lock(&sellLock);

//     User *user = users[issuerID];
//     vector<Trade *> ans;

//     if (user == NULL)
//     {
//         cout << "User ID not known" << endl;
//         return ans;
//     }

//     int remaining = amt;

//     // generate trades by matching the incoming order with pending buy orders
//     ans = generateTrades(false, price, issuerID, remaining);

//     if (remaining > 0)
//     { // put surplus amount on sell tree
//         putRemainingOrderOnTree(false, user, price, remaining);
//     }

//     pthread_mutex_unlock(&sellLock);
//     pthread_mutex_unlock(&buyLock);
//     pthread_mutex_unlock(&usersLock);

//     return ans;
// }

string TradeEngine::deleteBuyOrder(string username, long long orderId)
{
    //lazy deletion (?)
    pqxx::work W{C};
    json response;
    try
    {
        string query = "DELETE FROM ts.buy_orders WHERE order_id = \"" +
                       to_string(orderId) + "\" AND username = \"" + username + "\")";
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
        string query = "DELETE FROM ts.sell_orders WHERE order_id = \"" +
                       to_string(orderId) + "\" AND username = \"" + username + "\")";
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

// void TradeEngine::deleteSellOrder(string username, long long orderId)
// {
//     //lazy deletion
//     pqxx::work W{C};
//     string query = "DELETE FROM ts.sell_orders WHERE order_id = \"" +
//                    to_string(orderId) + "\" AND username = \"" + username + "\")";
//     W.exec(query);
//     W.commit();
// }

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

// vector<pair<int, int>> TradeEngine::getBuyVolumes()
// {

//     pthread_mutex_lock(&buyLock);

//     vector<pair<int, int>> v;

//     for (auto itr = buyTree.rbegin(); itr != buyTree.rend(); itr++)
//     {
//         int price = itr->first;
//         int vol = itr->second->first;
//         if (vol != 0)
//         {
//             v.push_back(pair<int, int>(price, vol));
//         }
//     }

//     pthread_mutex_unlock(&buyLock);

//     return v;
// }

// vector<pair<int, int>> TradeEngine::getSellVolumes()
// {

//     pthread_mutex_lock(&sellLock);

//     vector<pair<int, int>> v;

//     for (auto itr = sellTree.begin(); itr != sellTree.end(); itr++)
//     {
//         int price = itr->first;
//         int vol = itr->second->first;
//         if (vol != 0)
//         {
//             v.push_back(pair<int, int>(price, vol));
//         }
//     }

//     pthread_mutex_unlock(&sellLock);

//     return v;
// }

string TradeEngine::placeBuyOrder(int issuerID, int price, int amt)
{
    json response;
    try
    {
        pqxx::work W{C};
        int currAmt = amt;
        string query = 
            "SELECT * FROM ts.buy_orders WHERE price = (SELECT MIN(price) "
            "FROM ts.buy_orders) ORDER BY order_id ASC";
        pqxx::result R{W.exec(query)};
        json converted;
        while (R.size() && currAmt > 0)
        {

        }
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

string TradeEngine::placeSellOrder(int issuerID, int price, int amt)
{
    return "";
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

// vector<Order *> TradeEngine::getPendingBuyOrders(string username)
// {
//     pqxx::work W{C};
//     string query = "SELECT * FROM ts.buy_orders WHERE username = \"" +
//                    username + "\"";
//     pqxx::result R{W.exec(query)};
//     vector<Order *> ans;
//     for (auto row : R)
//     {
//         std::cout << row[0].c_str() << " | " << row[1].c_str() << '\n';
//         ans.push_back(new Order(true, row[2], row[3], row[1], row[0]));
//     }
//     W.commit();
// }

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

// vector<Trade *> *TradeEngine::getBuyTrades(int userID)
// {

//     pthread_mutex_lock(&usersLock);
//     User *user = users[userID];
//     pthread_mutex_unlock(&usersLock);

//     if (user == NULL)
//     {
//         cout << "User ID not known" << endl;
//         return nullptr;
//     }

//     return user->getBought();
// }

// vector<Trade *> *TradeEngine::getSellTrades(int userID)
// {

//     pthread_mutex_lock(&usersLock);
//     User *user = users[userID];
//     pthread_mutex_unlock(&usersLock);

//     if (user == NULL)
//     {
//         cout << "User ID not known" << endl;
//         return nullptr;
//     }

//     return user->getSold();
// }

// long long TradeEngine::getTotalVolume()
// {

//     long long totalVol = 0;

//     vector<pair<int, int>> buyTree = getBuyVolumes();
//     vector<pair<int, int>> sellTree = getSellVolumes();

//     for (int i = 0; i < buyTree.size(); i++)
//     {
//         totalVol += buyTree[i].second;
//     }

//     for (int i = 0; i < sellTree.size(); i++)
//     {
//         totalVol += sellTree[i].second;
//     }

//     for (auto itr = users.begin(); itr != users.end(); itr++)
//     {
//         User *user = itr->second;
//         vector<Trade *> *bought = user->getBought();
//         for (int i = 0; i < bought->size(); i++)
//         {
//             totalVol += bought->at(i)->getAmt();
//         }
//         vector<Trade *> *sold = user->getSold();
//         for (int i = 0; i < sold->size(); i++)
//         {
//             totalVol += sold->at(i)->getAmt();
//         }
//     }

//     return totalVol;
// }

// // helper methods defined below
// bool TradeEngine::firstOrderIsStale(list<Order *> *lst)
// {

//     Order *first = lst->front();

//     if (!first->checkValid())
//     {
//         lst->pop_front();
//         delete first;
//         return true;
//     }

//     return false;
// }

// void TradeEngine::putRemainingOrderOnTree(bool buyOrSell, User *user, int price, int remaining)
// {

//     map<int, pair<int, list<Order *> *> *> &tree = buyOrSell ? buyTree : sellTree;
//     pair<int, list<Order *> *> *p = tree[price];

//     Order *leftover = user->issueOrder(buyOrSell, price, remaining);

//     if (p != NULL)
//     {
//         p->first += remaining;
//         p->second->push_back(leftover);
//     }
//     else
//     {
//         list<Order *> *lst = new list<Order *>();
//         lst->push_back(leftover);
//         tree[price] = new pair<int, list<Order *> *>(remaining, lst);
//     }
// }

// vector<Trade *> TradeEngine::generateTrades(bool buyOrSell, int &price, int &issuerID, int &remaining)
// {

//     vector<Trade *> ans;

//     // generate trades when appropriate
//     if (buyOrSell)
//     {

//         for (auto itr = sellTree.begin(); itr != sellTree.end(); itr++)
//         {

//             int currPrice = itr->first;
//             if (currPrice > price || remaining <= 0)
//                 break;

//             int amtLeft = itr->second->first;
//             list<Order *> *orders = itr->second->second;

//             // consume pending sell orders at this price point
//             consumePendingOrders(true, issuerID, remaining, amtLeft, currPrice, orders, ans);

//             itr->second->first = amtLeft;
//         }
//     }
//     else
//     {

//         for (auto itr = buyTree.rbegin(); itr != buyTree.rend(); itr++)
//         {

//             int currPrice = itr->first;
//             if (currPrice < price || remaining <= 0)
//                 break;

//             int amtLeft = itr->second->first;
//             list<Order *> *orders = itr->second->second;

//             // consume pending buy orders at this price point
//             consumePendingOrders(false, issuerID, remaining, amtLeft, currPrice, orders, ans);

//             itr->second->first = amtLeft;
//         }
//     }

//     return ans;
// }

// void TradeEngine::consumePendingOrders(bool buyOrSell, int &issuerID, int &remaining, int &amtLeft,
//                                        int &currPrice, list<Order *> *orders, vector<Trade *> &ans)
// {

//     while (remaining > 0 && !orders->empty())
//     {

//         if (firstOrderIsStale(orders))
//             continue; // remove if current order is stale and continue

//         Order *first = orders->front();
//         int currAmt = first->getAmt();
//         int counterpartyID = first->getIssuerID();

//         // figure out who's the buyer and the seller
//         int buyerID = buyOrSell ? issuerID : counterpartyID;
//         int sellerID = buyOrSell ? counterpartyID : issuerID;
//         User *buyer = users[buyerID];
//         User *seller = users[sellerID];

//         User *counterparty = users[counterpartyID];

//         if (remaining < currAmt)
//         { // current order not finished

//             Trade *trade = new Trade(remaining, currPrice, buyerID, sellerID);
//             ans.push_back(trade);

//             // update buyer's and seller's finished orders
//             buyer->getBought()->push_back(trade);
//             seller->getSold()->push_back(trade);

//             // update first order amount
//             first->setAmt(currAmt - remaining);
//             amtLeft -= remaining;
//             remaining = 0;
//         }
//         else
//         { // current order finished

//             Trade *trade = new Trade(currAmt, currPrice, buyerID, sellerID);
//             ans.push_back(trade);

//             // update buyer's and seller's finished orders
//             buyer->getBought()->push_back(trade);
//             seller->getSold()->push_back(trade);
//             orders->pop_front();

//             // update counterparty's orders
//             counterparty->getOrders()->erase(first->getID());
//             delete first;
//             remaining -= currAmt;
//             amtLeft -= currAmt;
//         }
//     }
// }
