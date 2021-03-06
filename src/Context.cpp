#include "Context.h"
#include <algorithm>
#include <iostream>
#include <utility>
#include <cstdio>
#include <cstring>

extern const Item EMPTY{"000", ItemType::Terminal};
extern const Item Eof{"$", ItemType::Terminal};
using std::vector;
using std::set;
using std::string;
using std::pair;
using std::cout;
using std::endl;
using std::move;
using std::copy_if;
using std::back_inserter;
using std::begin;
using std::end;
using std::runtime_error;
using std::array;

Context::Context(vector<Production> grammar, Production startProduction) : ruleList(move(grammar)), firstSet{},
                                                                           followSet{},
                                                                           start(move(startProduction)) {

}

void Context::first() {
    bool hasChanged;
    for (auto &p : ruleList) {
        if (firstSet.find(p.getName()) == end(firstSet)) {
            firstSet.insert(pair<string, set<Item>>{p.getName(), set<Item>{}});
        }
        for (auto &i : p) {
            bool exists = firstSet.find(i.getName()) != end(firstSet);
            if (!exists && i.isNoTerminal()) {
                firstSet.insert(pair<string, set<Item>>{i.getName(), set<Item>{}});
            } else if (!exists && i.isTerminal()) {
                firstSet.insert(pair<string, set<Item>>{i.getName(), set<Item>{i}});
            }
        }
    }
    do {
        hasChanged = false;
        for (auto &p : ruleList) {
            set<Item> rhs{};
            if (p.size() < 1) {
                continue;
            }
            auto first = firstAt(p.first())->second;
            rhs.insert(begin(first), end(first));
            rhs.erase(EMPTY);
            int k = 0;
            if (p.size() > 1) {
                while (k < p.size() - 1 && isNullable(p[k])) {
                    auto nextFirst = firstAt(p[k + 1])->second;
                    rhs.insert(begin(nextFirst), end(nextFirst));
                    rhs.erase(EMPTY);
                    k++;
                }
                if (k == p.size() - 1 && isNullable(p[k])) {
                    rhs.insert(EMPTY);
                }
            } else if (p.first().isTerminal()) {
                rhs.emplace(p.first());
            }
            for (auto &i : rhs) {
                auto pa = p.getName();
                if (firstAt(pa) == end(firstSet)) {
                    throw runtime_error("invalid first element.");
                }
                auto &first2p = firstAt(pa)->second;
                if (first2p.find(i) == end(first2p)) {
                    hasChanged = true;
                    first2p.emplace(i);
                }
            }
        }
    } while (hasChanged);
}

void Context::follow() {
    const auto startItem = start.getItem();
    followSet.emplace(pair<string, set<Item>>(startItem.getName(), set<Item>{Eof}));

    bool hasChanged;
    do {
        hasChanged = false;
        for (auto &p : ruleList) {
            set<Item> result{};
            if (followSet.find(p.getName()) != followSet.end()) {
                result = followSet.find(p.getName())->second;
            }
            for (int j = p.size() - 1; j >= 0; j--) {
                auto &item = p[j];
                if (item.isNoTerminal()) {
                    if (followSet.find(item.getName()) == followSet.end()) {
                        followSet.insert(pair<string, set<Item>>(item.getName(), result));
                    } else {
                        auto &follow = followSet.find(item.getName())->second;
                        for (auto &i : result) {
                            if (follow.find(i) == follow.end()) {
                                hasChanged = true;
                                follow.emplace(i);
                            }
                        }
                    }
                    if (firstSet.find(item.getName()) == end(firstSet)) {
                        throw runtime_error("invalid item for first table");
                    }
                    auto &firstTable = firstSet.find(item.getName())->second;
                    if (firstTable.find(EMPTY) != firstTable.end()) {
                        result.insert(firstTable.begin(), firstTable.end());
                        result.erase(EMPTY);
                    } else {
                        result = firstTable;
                    }
                } else {
                    if (item == EMPTY) {
                        continue;
                    }
                    result = set<Item>{item};
                }
            }
        }
    } while (hasChanged);
}

bool Context::isNullable(const Item &item) {
    auto iterator = firstSet.find(item.getName());
    if (iterator == end(firstSet)) {
        return false;
    } else {
        return iterator->second.find(EMPTY) != iterator->second.end();
    }
}

void Context::printFirst() {
    for (auto &pair : firstSet) {
        cout << pair.first << " -> {";
        for (auto p = pair.second.begin(), n = pair.second.end(); p != pair.second.end(); p++) {
            cout << '\'' << p->getName() << '\'';
            n = p;
            n++;
            if (n != pair.second.end()) {
                cout << " ,";
            } else {
                cout << "}" << endl;
            }
        }
    }
}

void Context::printGrammar() {
    for (auto p : ruleList) {
        cout << p.getItem().getName() << " -> ";
        for (int i = 0; i < static_cast<int>(p.size()); i++) {
            cout << p[i].getName();
        }
        cout << endl;
    }
}

void Context::printFollow() {
    for (auto &pair : followSet) {
        cout << pair.first << " -> {";
        for (auto p = pair.second.begin(), n = pair.second.end(); p != pair.second.end(); p++) {
            cout << '\'' << p->getName() << '\'';
            n = p;
            n++;
            if (n != pair.second.end()) {
                cout << " ,";
            } else {
                cout << "}" << endl;
            }
        }
    }
}

auto Context::firstAt(const Item &item) -> decltype(firstSet.begin()) {
    return firstSet.find(item.getName());
}

auto Context::firstAt(const string &name) -> decltype(firstSet.begin()) {
    return firstSet.find(name);
}

bool Context::firstExist(const Item &item) {
    return firstAt(item) != firstSet.end();
}

bool Context::firstExist(const std::string &name) {
    return firstAt(name) != firstSet.end();
}

auto Context::followAt(const Item &item) -> decltype(followSet.begin()) {
    return followSet.find(item.getName());
}

auto Context::followAt(const string &name) -> decltype(followSet.begin()) {
    return followSet.find(name);
}

vector<HandlerSet> Context::generalLr1() {
    bool hasChanged;
    vector<HandlerSet> stateSet{};
    auto startHandler = Handler{start, 0, set<Item>{Eof}};
    vector<Handler> startHandlerVec{};
    startHandlerVec.reserve(100);
    closureSet(startHandler, startHandlerVec);
    HandlerSet firstHandler{Eof, startHandlerVec};
    firstHandler.setId(0);
    stateSet.emplace_back(firstHandler);
    do {
        try {

            hasChanged = false;
            for (auto p : stateSet) {
                auto nextStat = Goto(p);
                for (auto stat : nextStat) {
                    auto possPtr = std::find(stateSet.begin(), stateSet.end(), stat);
                    if (possPtr == stateSet.end()) {
                        hasChanged = true;
                        stat.setId(stateSet.size());
                        stat.setParentId(p.getId());
                        stateSet.emplace_back(stat);
                    }
                }
            }
        } catch (const std::exception &e) {
            cout << "exception";
        }
    } while (hasChanged);
    return stateSet;
}

vector<HandlerSet> Context::Goto(HandlerSet currState) {
    vector<HandlerSet> result{};
    set<Item> nTList{};
    set<Item> tList{};
    for (auto h : currState.ruleList()) {
        if (h.isEnd()) {
            continue;
        }
        auto item = h.current();
        if (item.isNoTerminal()) {
            nTList.emplace(item);
        } else if (item.isTerminal() && item.getName() != EMPTY.getName()) {
            tList.emplace(item);
        }
    }

    for (auto &nT : nTList) {
        vector<Handler> next{};
        for (auto h:currState.ruleList()) {
            if (!h.isEnd() && h.current() == nT) {
                next.emplace_back(move(h.nextHandler()));
            }
        }
        next = closureItemSet(next);
        result.emplace_back(HandlerSet{nT, next});
    }
    for (auto &t : tList) {
        vector<Handler> next{};
        for (auto h:currState.ruleList()) {
            if (!h.isEnd() && h.current() == t) {
                next.emplace_back(move(h.nextHandler()));
            }
        }
        next = closureItemSet(next);
        result.emplace_back(t, next);
    }
    return result;
}

vector<Handler> Context::closureItemSet(vector<Handler> &handlerSet) {
    vector<Handler> result{};
    result.reserve(20);
    for (auto handler : handlerSet) {
        auto closure = closureSet(handler, result);
    }
    return result;
}

vector<Handler> Context::closureSet(Handler &startHandler, vector<Handler> &result) {
    bool hasChanged;
    if (startHandler.getLookForward().empty()) {
        throw runtime_error("invalid start handler");
    }
    result.push_back(startHandler);
    do {
        hasChanged = false;
        for (auto currentHandler:result) {
            if (currentHandler.isEnd()) {
                continue;
            }
            auto item = currentHandler.current();
            auto bet = currentHandler.bet();
            if (item.isNoTerminal()) {
                auto pRules = rules(item);
                for (auto &p : pRules) {
                    Handler handler{p, 0};
                    if (!bet) {
                        handler.addLookForward(currentHandler.getLookForward().begin(),
                                               currentHandler.getLookForward().end());
                    } else {
                        auto &val = bet.value();
                        auto leftOpt = currentHandler.left();
                        auto &leftVal = leftOpt.value();
                        bool nullable = std::all_of(leftVal.begin(), leftVal.end(), [this](const Item &item) {
                            return isNullable(item);
                        });
                        if (val.isNoTerminal() && nullable) {
                            set<Item> lookItems{currentHandler.getLookForward().begin(),
                                                currentHandler.getLookForward().end()};
                            set<Item> firstSetAt{};
                            for (auto &leftItem : leftVal) {
                                lookItems.insert(firstAt(leftItem)->second.begin(), firstAt(leftItem)->second.end());
                            }
                            lookItems.insert(firstSetAt.begin(), firstSetAt.end());
                            lookItems.erase(EMPTY);
                            handler.addLookForward(lookItems.begin(), lookItems.end());
                        } else if (val.isTerminal()) {
                            handler.getLookForward().emplace(val);
                        } else {
                            set<Item> possLookItems{};
                            for (auto &nextItem: leftVal) {
                                auto firstAtItem = firstAt(nextItem);
                                bool nextIsEmpty = nextItem == EMPTY;
                                if ((nextItem.isNoTerminal() && isNullable(nextItem)) || nextIsEmpty) {
                                    possLookItems.insert(firstAtItem->second.begin(), firstAtItem->second.end());
                                } else if (nextItem.isNoTerminal()) {
                                    possLookItems.insert(firstAtItem->second.begin(), firstAtItem->second.end());
                                    break;
                                } else {
                                    possLookItems.emplace(nextItem);
                                    break;
                                }
                            }
                            possLookItems.erase(EMPTY);
                            handler.addLookForward(possLookItems.begin(), possLookItems.end());
                        }
                    }
                    auto ptr = find_if(result.begin(), result.end(), [&handler](Handler other) {
                        bool itemEqual = other.getItem() == handler.getItem();
                        if (!itemEqual) {
                            return false;
                        }
                        bool productionSizeEqual = other.getProduction().size() == handler.getProduction().size();
                        if (!productionSizeEqual) {
                            return false;
                        }
                        bool productionEqual = std::equal(handler.getProduction().begin(),
                                                          handler.getProduction().end(), other.getProduction().begin());
                        if (!productionEqual) {
                            return false;
                        }

                        return handler.getPosition() == other.getPosition();
                    });
                    if (ptr == result.end()) {

                        hasChanged = true;
                        result.emplace_back(handler);
                    } else {
                        auto &lookA1 = ptr->getLookForward();
                        vector<Item> diff{};
                        diff.reserve(10);
                        std::set_difference(handler.getLookForward().begin(), handler.getLookForward().end(),
                                            lookA1.begin(), lookA1.end(), std::back_inserter(diff));
                        if (!diff.empty()) {
                            hasChanged = true;
                            result[std::distance(result.begin(), ptr)].getLookForward().insert(diff.begin(),
                                                                                               diff.end());
                        }
                    }
                }
            }
        }
    } while (hasChanged);
    return
            result;
}

std::vector<Production> Context::rules(const Item &item) {
    vector<Production> result{};
    copy_if(ruleList.begin(), ruleList.end(), back_inserter(result), [&item](Production other) {
        return other.getName() == item.getName() && other.getItem().isTerminal() == item.isTerminal();
    });
    return result;
}

pair<Context::ActionTable, Context::GotoTable> Context::table(vector<HandlerSet> &state) {
    using ActionItem = array<int, 2>;
    using ItemName = string;
    using ItemAction = pair<ItemName, ActionItem>;
    using GotoAction = pair<ItemName, int>;
    // 1 for accept
    // 2 for shift
    // 3 for reduce
    auto gotoStat = [&](Item shift, vector<Handler> &handler) -> int {
        auto handleSet = HandlerSet{std::move(shift), handler};;
        auto nextGoto = Goto(handleSet);
        if (nextGoto.size() != 1) {
            throw runtime_error("invalid next state size");
        }
        for (auto &n : nextGoto) {
            auto ptr = std::find(state.begin(), state.end(), n);
            if (ptr == end(state)) {
                throw runtime_error("unknown goto state");
            }
            return std::distance(state.begin(), ptr);
        }
        return -1;
    };
    pair<Context::ActionTable, Context::GotoTable> result{Context::ActionTable{state.size()},
                                                          Context::GotoTable{state.size()}};
    auto &actionTable = result.first;
    auto &gotoTable = result.second;
    for (int i = 0; i < state.size(); i++) {
        auto &currState = state[i];
        auto &stateActionTable = actionTable[i];
        auto &stateGotoTable = gotoTable[i];
        for (auto item: currState.ruleList()) {
            int parent = currState.getParentId();
            if (item.isEnd() &&
                item.getItem() == start.getItem() &&
                item.getLookForward().size() == 1 &&
                item.getLookForward().find(Eof) != end(item.getLookForward())) {
                stateActionTable.insert(ItemAction{"$", ActionItem{1, 0}});
            } else if (item.isEnd()) {
                auto &lookForward = item.getLookForward();
                auto &production = item.getProduction();
                auto ptr = find_if(ruleList.begin(), ruleList.end(), [&production](Production &prod) {
                    if (production.size() != prod.size()) {
                        return false;
                    }
                    return prod.getName() == production.getName() &&
                           std::equal(prod.begin(), prod.end(), production.begin());
                });
                if (ptr == end(ruleList)) {
                    throw runtime_error("invalid production");
                }
                int id = std::distance(ruleList.begin(), ptr);
                for (auto &look : lookForward) {
                    stateActionTable.insert(ItemAction{look.getName(), ActionItem{3, id}});
                }
            } else if (!item.isEnd() && item.current().isTerminal() &&
                       std::any_of(item.getProduction().begin(), item.getProduction().end(),
                                   [](const Item &item) { return item == EMPTY; })) {
                auto &lookForward = item.getLookForward();
                auto &production = item.getProduction();
                auto ptr = find_if(ruleList.begin(), ruleList.end(), [&production](Production &prod) {
                    if (production.size() != prod.size()) {
                        return false;
                    }
                    return prod.getName() == production.getName() &&
                           std::equal(prod.begin(), prod.end(), production.begin());
                });
                if (ptr == end(ruleList)) {
                    throw runtime_error("invalid production");
                }
                int id = std::distance(ruleList.begin(), ptr);
                for (auto &look : lookForward) {
                    stateActionTable.insert(ItemAction{look.getName(), ActionItem{3, id}});
                }
            } else if (!item.isEnd() && item.current().isTerminal()) {
                vector<Handler> sameCurrentHandlerList{};;
                copy_if(currState.ruleList().begin(), currState.ruleList().end(),
                        std::inserter(sameCurrentHandlerList, sameCurrentHandlerList.end()),
                        [&item](Handler a1) {
                            return !a1.isEnd() && a1.current() == item.current();
                        });
                auto nextState = gotoStat(item.current(), sameCurrentHandlerList);
                stateActionTable.insert(ItemAction{item.current().getName(), ActionItem{2, nextState}});
            } else if (!item.isEnd() && item.current().isNoTerminal()) {
                vector<Handler> sameCurrentHandlerList{};;
                copy_if(currState.ruleList().begin(), currState.ruleList().end(),
                        std::inserter(sameCurrentHandlerList, sameCurrentHandlerList.end()),
                        [&item](Handler a1) {
                            return !a1.isEnd() && a1.current() == item.current();
                        });
                stateGotoTable.insert(
                        {GotoAction{item.current().getName(), gotoStat(item.current(), sameCurrentHandlerList)}});
            }
        }
    }
    return result;
}

void Context::printTable(pair<Context::ActionTable, Context::GotoTable> table) {
    set<string> nt{};
    set<string> t{Eof.getName()};
    for (auto &p : ruleList) {
        for (auto &i : p) {
            if (i.isNoTerminal() && std::find(nt.begin(), nt.end(), i.getName()) == end(nt)) {
                if (i == start.getItem()) {
                    continue;
                }
                nt.insert(i.getName());
            } else if (i.isTerminal() && std::find(t.begin(), t.end(), i.getName()) == end(t)) {
                if (i == EMPTY) {
                    continue;
                }
                t.insert(i.getName());
            }
        }
    }
    int actionTableLength = static_cast<int>(t.size() + 2) * 20;
    int gotoTableLength = static_cast<int>(nt.size()) * 20;
    ::printf("\n%*s|%-*s\n", actionTableLength, "action", gotoTableLength, "goto");
    ::printf("%10s", "state");
    for (auto &str: t) {
        printf("|%18s|", str.c_str());
    }
    printf("|");
    for (auto &str: nt) {
        printf("|%18s|", str.c_str());
    }
    printf("\n");
    char str[128];
    for (int i = 0; i < table.first.size(); i++) {
        auto &actionCurrent = table.first[i];
        auto &gotoCurrent = table.second[i];
        ::printf("%10d", i);
        for (auto &name : t) {
            memset(str, 0, sizeof(str));
            auto action = actionCurrent.find(name);
            if (action != std::end(actionCurrent)) {
                if (action->second[0] == 1) {
                    // accept
                    strcpy(str, "a");
                } else if (action->second[0] == 2) {
                    strcpy(str, "s");
                } else if (action->second[0] == 3) {
                    strcpy(str, "r");
                }
                ::printf("|%13s%-5d|", str, action->second[1]);
            } else {
                ::printf("|%18s|", "");
            }
        }
        ::printf("|");
        for (auto &name : nt) {
            auto gotoAction = gotoCurrent.find(name);
            if (gotoAction != std::end(gotoCurrent)) {
                printf("|%18d|", gotoAction->second);
            } else {
                printf("|%18s|", "");
            }
        }
        printf("\n");
    }

}

