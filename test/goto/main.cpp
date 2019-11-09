//
// Created by ftfunjth on 2019/11/8.
//

#include <gmock/gmock.h>
#include "../../src/Context.h"
#include <cstdio>

using namespace std;
using namespace testing;

class Goto : public Test {
public:
    vector<Item> itemList{
            Item{"S_", ItemType::NoTerminal},
            Item{"S", ItemType::NoTerminal},
            Item{"C", ItemType::NoTerminal},
            Item{"c", ItemType::Terminal},
            Item{"d", ItemType::Terminal},
    };
    Item &S_ = itemList[0];
    Item &S = itemList[1];
    Item &C = itemList[2];
    Item &c = itemList[3];
    Item &d = itemList[4];
    vector<Production> productions{
            Production{S_, vector<Item>{S}},
            Production{S, vector<Item>{C, C}},
            Production{C, vector<Item>{c, C}},
            Production{C, vector<Item>{d}},
    };

    Context context{productions, productions[0]};
};

TEST_F(Goto, ShouldGenerateLr1Table) {
    context.first();
    context.follow();
    auto result = context.generalLr1();
    auto table = context.table(result);
    for (auto n : result) {
        cout << endl;
        cout << "--- start new state " << n.shiftItem().getName() << " ---" << endl;
        for (auto h : n.ruleList()) {
            h.printHandler();
        }
        cout << "--- end new state --- " << endl;
    }

    ::printf("\n%25s|%10s\n", "action", "goto");
    ::printf("%10s%5s%5s%5s|%5s%5s\n", "state", "c", "d", "$", "S", "C");
    vector<string> nt{"c", "d", "$"};
    vector<string> t{"S", "C"};
    char str[16];
    for (int i = 0; i < result.size(); i++) {
        auto &actionCurrent = table.first[i];
        auto &gotoCurrent = table.second[i];
        ::printf("%10d", i);
        for (auto &name : nt) {
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
                ::printf("%3s%2d", str, action->second[1]);
            } else {
                ::printf("     ");
            }
        }
        ::printf("|");
        for (auto &name : t) {
            auto gotoAction = gotoCurrent.find(name);
            if (gotoAction != std::end(gotoCurrent)) {
                printf("%5d", gotoAction->second);
            } else {
                printf("     ");
            }
        }
        printf("\n");
    }
}

TEST_F(Goto, ShouldReturnNextHandlerSet) {
    context.first();
    context.follow();
    Item eof{"$", ItemType::Terminal};
    set<Item> lookAt{eof};
    Handler handler{productions[0], 0, lookAt};
    auto closure = context.closureSet(handler);
    HandlerSet startHandlerSet{Item{"", ItemType::Terminal}, closure};
    auto gotoNext = context.Goto(startHandlerSet);
    for (auto n : gotoNext) {
        cout << endl;
        cout << "--- start new state " << n.shiftItem().getName() << " ---" << endl;
        for (auto h : n.ruleList()) {
            h.printHandler();
        }
        cout << "--- end new state --- " << endl;
    }
}

int main(int argc, char *argv[]) {
    InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

