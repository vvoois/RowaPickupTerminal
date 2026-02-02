#pragma once
// ArticleManagement.h
// Centralized article handling: filtering, searching, and quantity management

#include <string>

namespace RowaPickupSlim::ArticleManagement
{
    /// Find the index of an article by ID in the current articles list
    /// @param articleId The ID of the article to find
    /// @return Index in current list, or -1 if not found
    int FindArticleIndex(const std::string& articleId);

    /// Update article quantity in both the filtered articles list and the full articles list
    /// This keeps both lists in sync so filtering doesn't lose quantity changes
    /// @param articleId The ID of the article to update
    /// @param newQty The new quantity (clamped to >= 0)
    void UpdateArticleQuantityInBothLists(const std::string& articleId, int newQty);

    /// Filter articles based on search text
    /// Performs case-insensitive substring matching against article IDs
    /// @param searchText The search string (will be converted to uppercase)
    /// @param resetSelection If true, selectedIndex will be reset to -1
    void FilterArticlesBySearchText(const std::string& searchText, bool resetSelection = true);

    /// Check if input is a complete article code (either "ROWA00020556" or "00020556")
    /// Used for scan output mode detection
    /// @param input The input string to check
    /// @return true if input matches complete article code format
    bool IsCompleteArticleCode(const std::string& input);

    /// Convert incomplete article code to complete format
    /// E.g., "00020556" -> "ROWA00020556"
    /// @param input The input string
    /// @return Complete article code in uppercase, or original input if already complete
    std::string ToCompleteArticleCode(const std::string& input);

    /// Find an article by complete article code (case-insensitive)
    /// @param completeCode The complete article code (e.g., "ROWA00020556")
    /// @param outId Output parameter for the article ID
    /// @param outQty Output parameter for the article quantity
    /// @return true if article found with quantity > 0
    bool FindArticleByCompleteCode(const std::string& completeCode, std::string& outId, int& outQty);

} // namespace RowaPickupSlim::ArticleManagement

