# iceoptions

## Introduction

This example demonstrates what kind of quality of service options can be configured on the publisher and subscriber
side. The options can be used for both, typed and untyped API flavours.

## Expected output

<!-- Add asciinema link here -->

## Code walkthrough

### Publisher

In order to configure a publisher, we have to supply a struct of the type `iox::popo::PublisherOptions` as a second parameter.

`historyCapacity` will enable subscribers to read the last n topics e.g. in case they are started later than the publisher:

```cpp
publisherOptions.historyCapacity = 10U;
```

Topics are automatically offered on creation of a publisher, if you want to disable that feature and control the offering yourself, do:

```cpp
publisherOptions.offerOnCreate = false;
```

To organize publishers inside an application, they can be associated and grouped by providing a node name. Some frameworks call nodes _runnables_.

```cpp
publisherOptions.nodeName = "Pub_Node_With_Options";
```

### Subscriber

To configure a subscriber, we have to supply a struct of the type `iox::popo::SubscriberOptions` as a second parameter.

The `queueCapacity` parameter specifies how many samples the queue of the subscriber object can hold. If the queue
would encounter an overflow, the oldest sample is released to create space for the newest one, which is then stored.

```cpp
subscriberOptions.queueCapacity = 10U;
```

`historyRequest` will enable a subscriber to receive the last n topics on subscription e.g. in case it was started later than the publisher. The publisher needs to have its `historyCapacity` enabled, too.

```cpp
subscriberOptions.historyRequest = 5U;
```

Topics are automatically subscribed on creation, if you want to disable that feature and control the subscription
yourself, set `subscribeOnCreate` appropriately:

```cpp
subscriberOptions.subscribeOnCreate = false;
```

Again, for organising subscribers inside an application, a `nodeName` can be applied:

```cpp
subscriberOptions.nodeName = "Sub_Node_With_Options";
```