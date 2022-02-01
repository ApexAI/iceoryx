// Copyright (c) 2022 by Apex.AI Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "test_popo_server_port_common.hpp"

namespace iox_test_popo_server_port
{
/// @todo iox-#27 remove before merge
// implemented tests:
// [x] initial isOffered with offer on create is true
// [x] initial isOffered with not offer on create is false
// [x] offer when already offered keeps isOffered true
// [x] offer when not yet offered changes isOffered to true
// [x] stopOffer when already offered changes isOffered to false
// [x] stopOffer when not yet offered keeps isOffered false
// [x] offer when there was intermediately a stopOffer results in isOffered true
// [x] stopOffer when there was intermediately an offer results in isOffered false

// [x] hasClients without offer is false
// [x] hasClients with no client is false
// [x] hasClients with client is true
// [x] hasClients with no client but intermediately having clients is false

// [x] hasNewRequests without offer is false
// [x] hasNewRequests with no requests is false
// [x] hasNewRequests with one requests is true
// [x] hasNewRequests with no request but previously having one is false
// [x] hasNewRequests with one request but intermediately having none is true
// [x] hasNewRequests multiple requests is true
// [x] hasNewRequests multiple requests and all but one removed is true
// [x] hasNewRequests with no requests but intermediately having multiple requests is false

// [x] getRequest without offer is ChunkReceiveResult::NO_CHUNK_AVAILABLE
// [x] getRequest with no requests is ChunkReceiveResult::NO_CHUNK_AVAILABLE
// [x] getRequest with requests results in RequestHeader
// [x] getRequest with no requests but previously having some is ChunkReceiveResult::NO_CHUNK_AVAILABLE
// [x] getRequest with requests but intermediately having none results in RequestHeader
// [x] getRequest with multiple requests results in as many RequestHeader as requests
// [x] getRequest with holding too many requests is ChunkReceiveResult::TOO_MANY_CHUNKS_HELD_IN_PARALLEL

// [x] releaseRequest of valid request
// [x] releaseRequest of invalid request
// [x] releaseRequest of nullptr request

// [x] hasLostRequestsSinceLastCall when there are no lost requests
// [x] hasLostRequestsSinceLastCall when there is one lost requests
// [x] hasLostRequestsSinceLastCall when there are multiple lost requests
// [x] hasLostRequestsSinceLastCall when there were lost requests but no new lost requests since last call

// [ ] allocateResponse with nullptr as request header returns error
// [ ] allocateResponse with invalid payload parameter returns error
// [ ] allocateResponse with valid parameter returns response header

// [ ] freeResponse with valid response
// [ ] freeResponse with invalid response

// [ ] sendResponse with nullptr as response header calls error handler
// [ ] sendResponse when not offered calls error handler
// [ ] sendResponse with invalid client queue id calls error handler
// [ ] sendResponse with full response queue calls error handler? check publisher behavior
// [ ] sendResponse with valid parameter works

// the tests below implicitly also test setConditionVariable and unsetConditionVariable
// [ ] isConditionVariableSet without condition variable is false
// [ ] isConditionVariableSet with condition variable is true
// [ ] isConditionVariableSet without condition variable but previously having a condition variable is false
// [ ] isConditionVariableSet with condition variable but intermediately having none is true

// BEGIN isOffered, offer and stopOffer tests

TEST_F(ServerPort_test, InitialIsOfferedOnPortWithOfferOnCreateIsTrue)
{
    ::testing::Test::RecordProperty("TEST_ID", "36800bff-c190-4e9f-b77c-de7a8530f35b");
    auto& sut = serverPortWithOfferOnCreate;
    EXPECT_TRUE(sut.portUser.isOffered());
}

TEST_F(ServerPort_test, InitialIsOfferedOnPortWithoutOfferOnCreateIsFalse)
{
    ::testing::Test::RecordProperty("TEST_ID", "ed3d652b-a77b-47a3-85c1-edb3042ad6f8");
    auto& sut = serverPortWithoutOfferOnCreate;
    EXPECT_FALSE(sut.portUser.isOffered());
}

TEST_F(ServerPort_test, OfferWhenAlreadyOfferedKeepsIsOfferedTrue)
{
    ::testing::Test::RecordProperty("TEST_ID", "fe3e2cac-df63-44c9-b333-c62db08713b8");
    auto& sut = serverPortWithOfferOnCreate;
    sut.portUser.offer();
    EXPECT_TRUE(sut.portUser.isOffered());
}

TEST_F(ServerPort_test, OfferWhenNotAlreadyOfferedChangesIsOfferedToTrue)
{
    ::testing::Test::RecordProperty("TEST_ID", "db41200b-fd65-4bbc-92b0-0e13d973c730");
    auto& sut = serverPortWithoutOfferOnCreate;
    sut.portUser.offer();
    EXPECT_TRUE(sut.portUser.isOffered());
}

TEST_F(ServerPort_test, StopOfferWhenAlreadyOfferedChangesIsOfferedToFalse)
{
    ::testing::Test::RecordProperty("TEST_ID", "71053a00-f93a-48fa-9fdb-eec9297fd6f1");
    auto& sut = serverPortWithOfferOnCreate;
    sut.portUser.stopOffer();
    EXPECT_FALSE(sut.portUser.isOffered());
}

TEST_F(ServerPort_test, StopOfferWhenNotOfferedKeepsIsOfferedFalse)
{
    ::testing::Test::RecordProperty("TEST_ID", "5fa3c4b1-7a0d-4b8d-9164-86574c019a65");
    auto& sut = serverPortWithoutOfferOnCreate;
    sut.portUser.stopOffer();
    EXPECT_FALSE(sut.portUser.isOffered());
}

TEST_F(ServerPort_test, OfferWhenThereIntermediatelyWasAStopOfferResultsInIsOfferedTrue)
{
    ::testing::Test::RecordProperty("TEST_ID", "0391523f-2c3b-4d7c-a329-3350f0659251");
    auto& sut = serverPortWithOfferOnCreate;
    sut.portUser.stopOffer();
    sut.portUser.offer();
    EXPECT_TRUE(sut.portUser.isOffered());
}

TEST_F(ServerPort_test, StopOfferWhenThereIntermediatelyWasAOfferResultsInIsOfferedFalse)
{
    ::testing::Test::RecordProperty("TEST_ID", "06c8ef10-798f-47fc-9146-5516ff40cffa");
    auto& sut = serverPortWithoutOfferOnCreate;
    sut.portUser.offer();
    sut.portUser.stopOffer();
    EXPECT_FALSE(sut.portUser.isOffered());
}

// END isOffered, offer and stopOffer tests

// BEGIN hasClients tests

TEST_F(ServerPort_test, HasClientsWithoutOfferIsFalse)
{
    ::testing::Test::RecordProperty("TEST_ID", "5a0d3ac7-fc4c-4d37-98af-ec4a3f8afe09");
    auto& sut = serverPortWithoutOfferOnCreate;
    EXPECT_FALSE(sut.portUser.hasClients());
}

TEST_F(ServerPort_test, HasClientsWithNoClientsIsFalse)
{
    ::testing::Test::RecordProperty("TEST_ID", "00e3f107-f9d1-42b7-b1ec-59daa0e275e5");
    auto& sut = serverPortWithOfferOnCreate;
    EXPECT_FALSE(sut.portUser.hasClients());
}

TEST_F(ServerPort_test, HasClientsWithClientIsTrue)
{
    ::testing::Test::RecordProperty("TEST_ID", "3e0e81dd-8d39-435f-b7a7-8f7121356fa4");
    auto& sut = serverPortWithOfferOnCreate;

    addClientQueue(sut);

    EXPECT_TRUE(sut.portUser.hasClients());
}

TEST_F(ServerPort_test, HasClientsWithNoClientsButIntermediatelyHavingClientsIsFalse)
{
    ::testing::Test::RecordProperty("TEST_ID", "2f890938-e6ae-4dce-a4e8-acbed1934153");
    auto& sut = serverPortWithOfferOnCreate;

    addClientQueue(sut);
    removeClientQueue(sut);

    EXPECT_FALSE(sut.portUser.hasClients());
}

// END hasClients tests

// BEGIN hasNewRequests tests

TEST_F(ServerPort_test, HasNewRequestsWithoutOfferIsFalse)
{
    ::testing::Test::RecordProperty("TEST_ID", "acac09bb-4e80-426a-b23b-16e55c1ce1c8");
    auto& sut = serverPortWithoutOfferOnCreate;
    EXPECT_FALSE(sut.portUser.hasNewRequests());
}

TEST_F(ServerPort_test, HasNewRequestsWithNoRequestsIsFalse)
{
    ::testing::Test::RecordProperty("TEST_ID", "e7aa181d-bd4f-46f5-ad57-2cc54a4ec93f");
    auto& sut = serverPortWithOfferOnCreate;
    EXPECT_FALSE(sut.portUser.hasNewRequests());
}

TEST_F(ServerPort_test, HasNewRequestsWithOneRequestIsTrue)
{
    ::testing::Test::RecordProperty("TEST_ID", "0235ce49-96b3-41c0-aac1-e0d6f6bc2b1f");
    auto& sut = serverPortWithOfferOnCreate;

    pushRequests(sut.requestQueuePusher, 1);

    EXPECT_TRUE(sut.portUser.hasNewRequests());
}

TEST_F(ServerPort_test, HasNewRequestsWithNoRequestsButPreviouslyHavingOneIsFalse)
{
    ::testing::Test::RecordProperty("TEST_ID", "29b0a347-2439-488d-ba13-962c7acffac1");
    auto& sut = serverPortWithOfferOnCreate;

    pushRequests(sut.requestQueuePusher, 1);
    IOX_DISCARD_RESULT(sut.portUser.getRequest());

    EXPECT_FALSE(sut.portUser.hasNewRequests());
}

TEST_F(ServerPort_test, HasNewRequestsWithOneRequestButIntermediatelyHavingNoneIsTrue)
{
    ::testing::Test::RecordProperty("TEST_ID", "a9fc0d89-7b97-48c4-8ba4-e4b22355116b");
    auto& sut = serverPortWithOfferOnCreate;

    pushRequests(sut.requestQueuePusher, 1);
    IOX_DISCARD_RESULT(sut.portUser.getRequest());
    pushRequests(sut.requestQueuePusher, 1);

    EXPECT_TRUE(sut.portUser.hasNewRequests());
}

TEST_F(ServerPort_test, HasNewRequestsWithMultipleRequestsIsTrue)
{
    ::testing::Test::RecordProperty("TEST_ID", "30e36e69-4f23-41ee-aac7-d34240f075ae");
    auto& sut = serverPortWithOfferOnCreate;


    pushRequests(sut.requestQueuePusher, 2);

    EXPECT_TRUE(sut.portUser.hasNewRequests());
}

TEST_F(ServerPort_test, HasNewRequestsWithMultipleRequestsAndAllButOneRemovedIsTrue)
{
    ::testing::Test::RecordProperty("TEST_ID", "96c42094-9931-46a3-8fd6-e7d28b490527");
    auto& sut = serverPortWithOfferOnCreate;

    pushRequests(sut.requestQueuePusher, 2);
    IOX_DISCARD_RESULT(sut.portUser.getRequest());

    EXPECT_TRUE(sut.portUser.hasNewRequests());
}

TEST_F(ServerPort_test, HasNewRequestsWithNoRequestsButIntermediatellyHavingMultipleRequestsIsFalse)
{
    ::testing::Test::RecordProperty("TEST_ID", "44845e16-ec0a-4b4e-a000-ff62f377c0b9");
    auto& sut = serverPortWithOfferOnCreate;

    pushRequests(sut.requestQueuePusher, 2);
    IOX_DISCARD_RESULT(sut.portUser.getRequest());
    IOX_DISCARD_RESULT(sut.portUser.getRequest());

    EXPECT_FALSE(sut.portUser.hasNewRequests());
}

// END hasNewRequests tests

// BEGIN getRequest tests

TEST_F(ServerPort_test, GetRequestWithoutOfferResultsIn_NO_CHUNK_AVAILABLE)
{
    ::testing::Test::RecordProperty("TEST_ID", "6c555224-8b03-46c5-a988-059ac2656149");
    auto& sut = serverPortWithoutOfferOnCreate;

    sut.portUser.getRequest()
        .and_then([&](const auto&) { FAIL() << "Expected ChunkReceiveResult::NO_CHUNK_AVAILABLE but got request"; })
        .or_else([&](const auto& error) { EXPECT_THAT(error, Eq(ChunkReceiveResult::NO_CHUNK_AVAILABLE)); });
}

TEST_F(ServerPort_test, GetRequestWithNoRequestsResultsIn_NO_CHUNK_AVAILABLE)
{
    ::testing::Test::RecordProperty("TEST_ID", "2b78536b-6902-4e55-aef2-654da5afdb80");
    auto& sut = serverPortWithOfferOnCreate;

    sut.portUser.getRequest()
        .and_then([&](const auto&) { FAIL() << "Expected ChunkReceiveResult::NO_CHUNK_AVAILABLE but got request"; })
        .or_else([&](const auto& error) { EXPECT_THAT(error, Eq(ChunkReceiveResult::NO_CHUNK_AVAILABLE)); });
}

TEST_F(ServerPort_test, GetRequestWithOneRequestsResultsInRequestHeader)
{
    ::testing::Test::RecordProperty("TEST_ID", "b3b79e97-00bb-4e36-931d-88cfbe027a07");
    auto& sut = serverPortWithOfferOnCreate;

    constexpr uint64_t REQUEST_DATA{42};
    pushRequests(sut.requestQueuePusher, 1, REQUEST_DATA);

    sut.portUser.getRequest()
        .and_then([&](const auto& req) { EXPECT_THAT(getRequestData(req), Eq(REQUEST_DATA)); })
        .or_else([&](const auto& error) { FAIL() << "Expected RequestHeader but got error: " << error; });
}

TEST_F(ServerPort_test, GetRequestWithNoRequestsButIntermediatelyHavingOneResultsIn_NO_CHUNK_AVAILABLE)
{
    ::testing::Test::RecordProperty("TEST_ID", "a1df6ee7-a936-446d-8960-5b9d6a93ad62");
    auto& sut = serverPortWithOfferOnCreate;

    pushRequests(sut.requestQueuePusher, 1);
    IOX_DISCARD_RESULT(sut.portUser.getRequest());

    sut.portUser.getRequest()
        .and_then([&](const auto&) { FAIL() << "Expected ChunkReceiveResult::NO_CHUNK_AVAILABLE but got request"; })
        .or_else([&](const auto& error) { EXPECT_THAT(error, Eq(ChunkReceiveResult::NO_CHUNK_AVAILABLE)); });
}

TEST_F(ServerPort_test, GetRequestWithOneRequestsButIntermediatelyHavingNoneResultsInRequestHeader)
{
    ::testing::Test::RecordProperty("TEST_ID", "ea4154a8-5a46-4c5d-b9cb-91adf6c1ff75");
    auto& sut = serverPortWithOfferOnCreate;

    constexpr uint64_t REQUEST_DATA_1{13};
    constexpr uint64_t REQUEST_DATA_2{73};

    pushRequests(sut.requestQueuePusher, 1, REQUEST_DATA_1);
    IOX_DISCARD_RESULT(sut.portUser.getRequest());
    pushRequests(sut.requestQueuePusher, 1, REQUEST_DATA_2);

    sut.portUser.getRequest()
        .and_then([&](const auto& req) { EXPECT_THAT(getRequestData(req), Eq(REQUEST_DATA_2)); })
        .or_else([&](const auto& error) { FAIL() << "Expected RequestHeader but got error: " << error; });
}

TEST_F(ServerPort_test, GetRequestWithMultipleRequestsResultsInAsManyRequestHeaderAsRequests)
{
    ::testing::Test::RecordProperty("TEST_ID", "35032bbb-ec59-4b54-ba27-bdb364105162");
    auto& sut = serverPortWithOfferOnCreate;

    constexpr uint64_t REQUEST_DATA_BASE{37};

    pushRequests(sut.requestQueuePusher, 2, REQUEST_DATA_BASE);

    sut.portUser.getRequest()
        .and_then([&](const auto& req) { EXPECT_THAT(getRequestData(req), Eq(REQUEST_DATA_BASE)); })
        .or_else([&](const auto& error) { FAIL() << "Expected RequestHeader but got error: " << error; });

    sut.portUser.getRequest()
        .and_then([&](const auto& req) { EXPECT_THAT(getRequestData(req), Eq(REQUEST_DATA_BASE + 1)); })
        .or_else([&](const auto& error) { FAIL() << "Expected RequestHeader but got error: " << error; });
}

TEST_F(ServerPort_test, GetRequestWithMaximalHeldChunksInParallelResultsInRequestHeader)
{
    ::testing::Test::RecordProperty("TEST_ID", "19c19b39-2dd1-4784-a1c1-adfba56248e8");
    auto& sut = serverPortWithOfferOnCreate;

    constexpr uint64_t REQUEST_DATA_BASE{7337};
    // the maximum number of request which can be held in parallel must be larger than
    // MAX_REQUESTS_PROCESSED_SIMULTANEOUSLY; if it would be the same, the server would have to release one request
    // before a new one could be fetched and for a short time window the requirement of being able to hold
    // MAX_REQUESTS_PROCESSED_SIMULTANEOUSLY would be broken
    constexpr uint64_t MAX_REQUEST_HELD_IN_PARALLEL = iox::MAX_REQUESTS_PROCESSED_SIMULTANEOUSLY + 1;

    pushRequests(sut.requestQueuePusher, MAX_REQUEST_HELD_IN_PARALLEL, REQUEST_DATA_BASE);

    std::vector<const RequestHeader*> responsesToProcess;
    for (uint64_t i = 0; i < MAX_REQUEST_HELD_IN_PARALLEL; ++i)
    {
        sut.portUser.getRequest()
            .and_then([&](const auto& req) {
                EXPECT_THAT(getRequestData(req), Eq(REQUEST_DATA_BASE + i));
                responsesToProcess.push_back(req);
            })
            .or_else([&](const auto& error) { FAIL() << "Expected RequestHeader but got error: " << error; });
    }
}

TEST_F(ServerPort_test, GetRequestWhenProcessingTooManyRequestsInParallelResultsIn_TOO_MANY_CHUNKS_HELD_IN_PARALLEL)
{
    ::testing::Test::RecordProperty("TEST_ID", "19c19b39-2dd1-4784-a1c1-adfba56248e8");
    auto& sut = serverPortWithOfferOnCreate;

    constexpr uint64_t REQUEST_DATA_BASE{7337};
    // the maximum number of request which can be held in parallel must be larger than
    // MAX_REQUESTS_PROCESSED_SIMULTANEOUSLY; if it would be the same, the server would have to release one request
    // before a new one could be fetched and for a short time window the requirement of being able to hold
    // MAX_REQUESTS_PROCESSED_SIMULTANEOUSLY would be broken
    constexpr uint64_t MAX_REQUEST_HELD_IN_PARALLEL = iox::MAX_REQUESTS_PROCESSED_SIMULTANEOUSLY + 1;

    pushRequests(sut.requestQueuePusher, MAX_REQUEST_HELD_IN_PARALLEL + 1, REQUEST_DATA_BASE);

    std::vector<const RequestHeader*> responsesToProcess;
    for (uint64_t i = 0; i < MAX_REQUEST_HELD_IN_PARALLEL; ++i)
    {
        IOX_DISCARD_RESULT(sut.portUser.getRequest());
    }

    sut.portUser.getRequest()
        .and_then([&](const auto&) {
            FAIL() << "Expected ChunkReceiveResult::TOO_MANY_CHUNKS_HELD_IN_PARALLEL but got request";
        })
        .or_else(
            [&](const auto& error) { EXPECT_THAT(error, Eq(ChunkReceiveResult::TOO_MANY_CHUNKS_HELD_IN_PARALLEL)); });
}

// END getRequest tests

// BEGIN releaseRequest tests

TEST_F(ServerPort_test, ReleaseRequestWithValidRequestHeaderWorksAndReleasesTheChunkToTheMempool)
{
    ::testing::Test::RecordProperty("TEST_ID", "ffb5df3f-2f2d-40a9-b0b6-53b44daa5568");
    auto& sut = serverPortWithOfferOnCreate;

    constexpr uint64_t REQUEST_DATA{42};
    pushRequests(sut.requestQueuePusher, 1, REQUEST_DATA);

    sut.portUser.getRequest()
        .and_then([&](const auto& req) {
            EXPECT_THAT(getNumberOfUsedChunks(), Eq(1U));
            sut.portUser.releaseRequest(req);
            EXPECT_THAT(getNumberOfUsedChunks(), Eq(0U));
        })
        .or_else([&](const auto& error) { FAIL() << "Expected RequestHeader but got error: " << error; });
}

TEST_F(ServerPort_test, ReleaseRequestWithInvalidRequestHeaderCallsTheErrorHandler)
{
    ::testing::Test::RecordProperty("TEST_ID", "63bf7750-3193-4711-baa8-6cee500297da");
    auto& sut = serverPortWithOfferOnCreate;

    auto sharedChunk = getChunkWithInitializedRequestHeaderAndData();

    iox::cxx::optional<iox::Error> detectedError;
    auto errorHandlerGuard = iox::ErrorHandler::setTemporaryErrorHandler(
        [&](const iox::Error error, const std::function<void()>, const iox::ErrorLevel errorLevel) {
            EXPECT_THAT(error, Eq(iox::Error::kPOPO__CHUNK_RECEIVER_INVALID_CHUNK_TO_RELEASE_FROM_USER));
            EXPECT_THAT(errorLevel, Eq(iox::ErrorLevel::SEVERE));
            detectedError.emplace(error);
        });

    sut.portUser.releaseRequest(static_cast<const RequestHeader*>(sharedChunk.getChunkHeader()->userHeader()));

    EXPECT_TRUE(detectedError.has_value());
}

TEST_F(ServerPort_test, ReleaseRequestWithNullptrRequestHeaderCallsTheErrorHandler)
{
    ::testing::Test::RecordProperty("TEST_ID", "b505019f-ba47-4df4-ba5e-d2d16e5c44cd");
    auto& sut = serverPortWithOfferOnCreate;

    iox::cxx::optional<iox::Error> detectedError;
    auto errorHandlerGuard = iox::ErrorHandler::setTemporaryErrorHandler(
        [&](const iox::Error error, const std::function<void()>, const iox::ErrorLevel errorLevel) {
            EXPECT_THAT(error, Eq(iox::Error::kEXPECTS_ENSURES_FAILED));
            EXPECT_THAT(errorLevel, Eq(iox::ErrorLevel::FATAL));
            detectedError.emplace(error);
        });

    sut.portUser.releaseRequest(nullptr);

    EXPECT_TRUE(detectedError.has_value());
}

// END releaseRequest tests

// BEGIN hasLostRequestsSinceLastCall tests

TEST_F(ServerPort_test, HasLostRequestsSinceLastCallWhenNoRequestsAreLostReturnsFalse)
{
    ::testing::Test::RecordProperty("TEST_ID", "134a100a-e28e-461e-9860-635a85589cc1");
    auto& sut = serverPortWithOfferOnCreate;
    EXPECT_FALSE(sut.portUser.hasLostRequestsSinceLastCall());
}

TEST_F(ServerPort_test, HasLostRequestsSinceLastCallWithFullQueueReturnsFalse)
{
    ::testing::Test::RecordProperty("TEST_ID", "5c7aff9a-2460-4b7a-9b78-8ca5a4f80503");
    auto& sut = serverPortWithOfferOnCreate;

    pushRequests(sut.requestQueuePusher, QUEUE_CAPACITY);

    EXPECT_FALSE(sut.portUser.hasLostRequestsSinceLastCall());
}

TEST_F(ServerPort_test, HasLostRequestsSinceLastCallWhenOneRequestIsLostReturnsTrue)
{
    ::testing::Test::RecordProperty("TEST_ID", "8a300681-4420-45be-8241-cb172ca61561");
    auto& sut = serverPortWithOfferOnCreate;

    pushRequests(sut.requestQueuePusher, QUEUE_CAPACITY + 1);

    EXPECT_TRUE(sut.portUser.hasLostRequestsSinceLastCall());
}

TEST_F(ServerPort_test, HasLostRequestsSinceLastCallWhenMultipleRequestAreLostReturnsTrue)
{
    ::testing::Test::RecordProperty("TEST_ID", "2235a5cf-cf04-42f1-929e-20e0f73bc2f5");
    auto& sut = serverPortWithOfferOnCreate;

    pushRequests(sut.requestQueuePusher, QUEUE_CAPACITY + 2);

    EXPECT_TRUE(sut.portUser.hasLostRequestsSinceLastCall());
}

TEST_F(ServerPort_test, HasLostRequestsSinceLastCallWhenNoFurtherRequestAreLostReturnsFalse)
{
    ::testing::Test::RecordProperty("TEST_ID", "27a8ddcc-e22b-4bd9-abbd-ed61ac88d01c");
    auto& sut = serverPortWithOfferOnCreate;

    pushRequests(sut.requestQueuePusher, QUEUE_CAPACITY + 1);
    IOX_DISCARD_RESULT(sut.portUser.hasLostRequestsSinceLastCall());

    EXPECT_FALSE(sut.portUser.hasLostRequestsSinceLastCall());
}

TEST_F(ServerPort_test, HasLostRequestsSinceLastCallWhenFurtherRequestAreLostReturnsTrue)
{
    ::testing::Test::RecordProperty("TEST_ID", "21fd5e5e-93e4-4ef9-8448-d1c6ae6e3989");
    auto& sut = serverPortWithOfferOnCreate;

    pushRequests(sut.requestQueuePusher, QUEUE_CAPACITY + 1);
    IOX_DISCARD_RESULT(sut.portUser.hasLostRequestsSinceLastCall());
    pushRequests(sut.requestQueuePusher, 1);

    EXPECT_TRUE(sut.portUser.hasLostRequestsSinceLastCall());
}

TEST_F(ServerPort_test, HasLostRequestsSinceLastCallWhenNoRequestAreLostAfterRemovingRequestFromQueueReturnsTrue)
{
    ::testing::Test::RecordProperty("TEST_ID", "248ddc67-5717-4b33-8b04-80d8246fedb3");
    auto& sut = serverPortWithOfferOnCreate;

    pushRequests(sut.requestQueuePusher, QUEUE_CAPACITY + 1);
    IOX_DISCARD_RESULT(sut.portUser.hasLostRequestsSinceLastCall());
    IOX_DISCARD_RESULT(sut.portUser.getRequest());
    pushRequests(sut.requestQueuePusher, 1);

    EXPECT_FALSE(sut.portUser.hasLostRequestsSinceLastCall());
}

// END hasLostRequestsSinceLastCall tests

} // namespace iox_test_popo_server_port
