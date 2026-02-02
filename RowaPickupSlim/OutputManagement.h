#pragma once
// OutputManagement.h
// Centralized output (order) request handling

#include <string>

namespace RowaPickupSlim::OutputManagement
{
    /// Update output records based on order status from server
    /// Handles color assignment based on status and ownership
    /// If status is "Completed", removes the record and adjusts article quantities
    /// @param orderId The unique output/order ID
    /// @param articleId The article ID
    /// @param quantityRequested Quantity requested in the order
    /// @param packsDelivered Number of packs that have been delivered
    /// @param status Current status ("Queued", "InProcess", "Completed", "Rejected", "Incomplete")
    /// @param isOurOutput true if we sent this request, false if another client sent it
    void UpdateOutputRecordFromMessage(const std::string& orderId, const std::string& articleId,
                                      int quantityRequested, int packsDelivered,
                                      const std::string& status, bool isOurOutput);

    /// Send an OutputRequest for a single article
    /// Creates a unique request ID and tracks it in g_state.ourOutputRequestIds
    /// Adds a local output record marked as Queued (Blue) for immediate UI feedback
    /// @param articleId The article to request
    /// @param qty The quantity requested
    void SendOutputRequestForArticle(const std::string& articleId, int qty);

    /// Send OutputRequest for all articles with quantities > 0
    /// Sends individual requests for each article
    void SendOutputRequestForAllArticles();

} // namespace RowaPickupSlim::OutputManagement

