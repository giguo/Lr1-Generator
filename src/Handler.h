//
// Created by 曹高阳 on 2019-11-08.
//

#ifndef FIRST_FOLLOW_HANDLER_H
#define FIRST_FOLLOW_HANDLER_H

#include "Common.h"
#include "Production.h"
#include <exception>
#include <optional>

class Handler {
private:
    Production production;
    std::set<Item> lookForward;
    size_t position;

public:
    explicit Handler(Production p, size_t position = 0);

    Handler(Production p, size_t pos, std::set<Item> lookTable);

    std::vector<Item> alpha();

    Item last() {
        return production.last();
    }

    auto B() -> decltype(production.handleList.begin());

    std::optional<Item> bet();

    std::optional<std::vector<Item>> left();

    void printHandler();

    bool isEnd();

    Handler nextHandler();

    bool operator<(const Handler &handler) const;

    bool operator==(const Handler &handler) const;

    void addLookForward(std::set<Item>::const_iterator begin, std::set<Item>::const_iterator end);

    Item current();

    std::set<Item> &getLookForward();

    Item getItem();

    Production &getProduction();

    int getPosition();
};


#endif //FIRST_FOLLOW_HANDLER_H
