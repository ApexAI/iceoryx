# StatusPort

## Summary and requirements

The `StatusPort` is an alternative to the publish subscriber communication in `iceoryx_posh`.

The target of the `StatusPort` are use-case with the following properties:

* Data is transferred once or is rarely updated
* There are many readers which are interested in the data
* Data can be persistent
    * The lifetime of the transferred data is bound to the lifetime of `StatusPortData`
* Readers don't access the data directly but via a lambda
    * Preventing torn reads, since the `StatusPortReader` detects if the data
    changed during `take()` operation (Frankenstein check) and re-executes the lambda
* Has to be attachable to `Listener`

Potential applications are:

* Introspection topics
* Service Discovery

## Design

* Only one `StatusPortData` object shared between `StatusPortReader` and `StatusPortWriter`.

// wie können wir hier sicherstellen, dass niemand mehr auf dieser speicherzelle liest zB ein gaaanz langsamer
// Leser? brauche ich einen referenceCounter? nein, der leser checkt ob sich die welt weitergedreht hat

### Discarded ideas & design alternatives

#### Atomic pointer
The pointer to the currently active and used chunk could also be stored in an `std::atomic`.

```cpp
std::atomic<T*> activeChunk{nullptr}
```

However, it would need the full 64-bit and which is not needed when managing
just two chunks. Hence an `abaCounter` would need to be stored in a separate
`std::atomic` variable.

### Omit `ServiceDescription`

* No discovery, no `CaPro`, no QoS

### atomic<T>::exchange

* Not possible because we are not allowed to write data into shared memory as a reader

### Class structure

* Unterscheidung zwischen Writer/ Reader oder User/RouDi?
    * Nein
    * Only RouDi is allowed to acquire `StatusPortWriter`
* Soll ich vom BasePort ableiten?
    * Nein
    * Brauche ich den Custom User Header im Status Port?
    * Ja!

### Terminology

* Transaction: Atomic state of the world, which is changed by a write operation
* Current transaction: Transaction in local scope, read in the beginning of each operation
* Latest transaction: Transaction in the shared memory managment segment
* Chunk: Untyped piece of memory located in the shared memory payload segment
* Read position: The chunk, which was used the last to write data
* Write position: The opposite chunk not currently being used by the `StatusPortReader`s

### Contract & Properties

* Two memory chunks from shared memory payload segment are used
* 1:N
* Data exchanged needs to be trivially-copyable
* Storing a chunk cannot fail
    * binary world view
    * two array entries
    * binary swap
* Reading a chunk cannot fail
    * If no data was sent yet, `callable` is not called
* Not part of the user API, only used internally
* Reader only needs write access to the data structure and no read access
    * Same in TACO? TACO is only one shared data structure; in TACO both reader
    and write data
    * Is this actually possible? How can the reader communicate that he's still
    reading the data?
* ~~Writer owns the memory~~
* Writing and Reading will be tried indefinitely till possible,
  hence starvation is possible
* `StatusPortData` is created in the shared memory segment if either a `StatusPortWriter`
  or `StatusPortReader` is created
    * Two chunks in the shared memory payload segment are bound to the lifetime
      of the `StatusPortData` (either via `StatusPortData` c'tor or `StatusPort{Writer,Reader}` c'tor)
* Users can only acquire `StatusPortReader`

### Considerations

### Solution

* Create `.puml`'s

#### Frankenstein object corner cases

Let's assume that that there is one `StatusPortWriter` and one `StatusPortReader`
in this scenario. Further assume that the type transferred via the
`StatusPort` is

```cpp
T = cxx::string<36>
```

The following samples are transmitted by the `StatusPortWriter`

1. "Time And Relative Dimension In Space"
2. "Twin Ion Engines Fighter"
3. "USS Enterprise"

After the second transaction the `StatusPortReader` see's the latest transaction
below and starts reading the `chunk[1]` aka `SECOND`.

```text
Latest transaction in shared memory managment segment
+----------------------------------+
|       ActiveChunk::SECOND        |
|       abaCounter: 1              |
+----------------------------------+
```

Now the the `StatusPortWriter` starts writing concurrently the third sample and
overtakes the slower `StatusPortReader`.
Due the latest transaction depicted below, `writePosition` is calculated to
chunks[1] aka "SECOND". Below you can find the location `^` at which the writer is
currently writing and the reader is reading. As a result the strings are mixed
up at this very moment in time, a so called Frankenstein object. While is this
case, the reader might still read uncorrupted data if he finished before the
writer, one can easily imagine that this can to very subtile, nasty bugs.
To avoid this problem the `StatusPortReader` compares the initial transaction
with the latest transaction and  re-calls the callable as long as they are not
the same. The memory order `std::memory_order_acquire` ensures that the
operations below the `load()` "happened-before" and the memory of `chunks[]` is synchronized.

```text
Shared memory payload segment
+----------------------------------------------------+
|        SharedChunk chunks[2]                       |
|        +----------------------------------------+  |
| FIRST  | "Twin Ion Engines Fighter"             |  |
|        +----------------------------------------+  |
| SECOND | "USS Enterprlative Dimension In Space" |  |
|        +----------------------------------------+  |
|                      ^                 ^           |
|                      Writer            Slow reader |
+----------------------------------------------------+

Latest transaction in shared memory managment segment
+----------------------------------+
|       ActiveChunk::FIRST         |
|       abaCounter: 2              |
+----------------------------------+
```



### Code example

```cpp
```

## Todo

* Um `popo::Sample` verwenden zu können muss entweder das `PublisherInterface`
  implementiert oder refactored werden

## Open issues

* In welchem Shared Memory Segment sollen die Daten liegen?
    * Verwendet der `StatusPort` den normalen Memory Manager?
        * Eher nein, oder?
* Wird `StatusPortData` zwischen `StatusPortWriter` und `StatusPortReader` geteilt?
    * Wie finden Sie sich, wenn es kein `CaPro` gibt? Über den Datentyp?
        * Wir brauchen CaPro, es ist auch ein Service, wenn auch ein sehr spezieller
    * Wann matchen sie?
* Name `StatusPort{Writer,Reader}` just `StatusReader` and `StatusWriter`?
* Does [`copyTake()`](LINK_TO_GIST) make sense as an additional contract with
  the user?
