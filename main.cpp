/*
 *  Author: a7mddra
 */

// Personalized unbeatable tic tac toe

/*
    Core MiniMax idea:

    Layer 1: correctness
    --------------------
    outcome:
        AI win      > draw > AI loss

    This layer must dominate everything else.
    Never choose a fancy draw over a forced win.
    Never choose a stylish loss over a draw.


    Layer 2: personality / tie-breakers
    -----------------------------------
    win speed:
        faster AI win is better

    loss delay:
        slower AI loss is better

    win count:
        how many leaves end with AI winning

    draw count:
        how many leaves end with draw

    loss count:
        how many leaves end with AI losing

    threats made:
        immediate win threats / forks created by AI

    threats allowed:
        opponent immediate wins / opponent forks allowed

    position value:
        center > corner > edge

    random salt:
        tiny randomness among truly equal moves


    Final personality:
    ------------------
    1. Never lose if a draw/win is possible.
    2. Win if a forced win exists.
    3. Prefer faster wins.
    4. Prefer slower losses if losing is unavoidable.
    5. Among equal outcomes, choose the move that creates more pressure.
    6. Among still-equal moves, choose better position.
    7. Randomize only among truly equal moves.
*/

#include <bits/stdc++.h>
using namespace std;

using ll = long long;
using ld = long double;

const string el = "\n";

const ll INF = 1e18;
const ll MOD = 1e9 + 7;
// const ll MOD = 998244353;

#include "app.h"

void ip(bool INTERACTIVE) {
#if LOCAL
    if (!INTERACTIVE) {
        freopen("input.txt", "r", stdin);
    }
#endif
}

void run(ll tc) {
    app();
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    ip(false);

    ll T = 1;
    ll i = 0;

    // cin >> T;

    while (i < T) {
        run(i + 1);
        i++;
    }

    return 0;
}
