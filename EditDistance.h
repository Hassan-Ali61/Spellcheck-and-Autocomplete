#pragma once

#include <algorithm>
#include <string>

constexpr int MAX_WORD_LENGTH = 50;      // size limit of the fixed DP table
constexpr int DISTANCE_TOO_FAR = 100000; // returned when a word is too long to compare

// Minimum number of insertions, deletions and replacements needed to turn a into b.
inline int editDistance(const std::string &a, const std::string &b)
{
    const int m = static_cast<int>(a.size());
    const int n = static_cast<int>(b.size());
    if (m > MAX_WORD_LENGTH || n > MAX_WORD_LENGTH)
        return DISTANCE_TOO_FAR;

    // dp[i][j] = edits needed to turn the first i letters of a into the first j letters of b
    int dp[MAX_WORD_LENGTH + 1][MAX_WORD_LENGTH + 1];

    for (int i = 0; i <= m; i++)
        dp[i][0] = i;
    for (int j = 0; j <= n; j++)
        dp[0][j] = j;

    for (int i = 1; i <= m; i++)
    {
        for (int j = 1; j <= n; j++)
        {
            if (a[i - 1] == b[j - 1])
                dp[i][j] = dp[i - 1][j - 1];
            else
                dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
        }
    }

    return dp[m][n];
}
