# StatusPort

## Summary and requirements

The `StatusPort` is an alternative to the publish subscriber communication in `iceoryx_posh`.

The target of the `StatusPort` are the following use-case:

* Data is transferred once or is rarely updated
* There are many readers which are interested in the data
* Data can be persistent
    * The lifetime of the transferred data is bound to the lifetime of `StatusPortData`
* Readers don't access the data directly but by a lambda
    * Preventing torn reads, since the `StatusPortReader` detects if the data
    changed during `take()` operation (Frankenstein check) and re-executes the lambda

Potential applications are:

* Introspection topics
* Service Discovery

## Design

* Only one `StatusPortData` object shared between `StatusPortReader` and `StatusPortWriter`.

// wie können wir hier sicherstellen, dass niemand mehr auf dieser speicherzelle liest zB ein gaaanz langsamer
// Leser? brauche ich einen referenceCounter? nein, der leser checkt ob sich die welt weitergedreht hat

### Discarded ideas

The pointer to the currently active and used chunk could also be stored in an `std::atomic`.

```cpp
std::atomic<T*> activeChunk{nullptr}
```

However, it would need the full 64-bit and which is not needed when managing
just two chunks. Hence an `abaCounter` would need to be stored in a separate
`std::atomic` variable.

* No discovery, no `CaPro`, no QoS

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
(* Only RouDi is allowed to acquire `StatusPortWriter`)

### Considerations

### Solution

* Create `.puml`'s

### Code example

```cpp
```

## Todo

* Um `popo::Sample` verwenden zu können muss entweder das `PublisherInterface`
  implementiert oder refactored werden

## Open issues

* Brauche ich den Custom User Header im Status Port?
    * Ja!
* Unterscheidung zwischen Writer/ Reader oder User/RouDi?
    * Nein
* Soll ich vom BasePort ableiten?
* In welchem Shared Memory Segment sollen die Daten liegen?
    * Verwendet der `StatusPort` den normalen Memory Manager?
        * Eher nein, oder?
* Wird `StatusPortData` zwischen `StatusPortWriter` und `StatusPortReader` geteilt?
    * Wie finden Sie sich, wenn es kein `CaPro` gibt? Über den Datentyp?
        * Wir brauchen CaPro, es ist auch ein Service, wenn auch ein sehr spezieller
    * Wann matchen sie?
* Possible optimizations
    * Make all atomic operations relaxed and use an `abaCounter`
    * Make all atomic operations aquire-releases without using an `abaCounter`
* Name `StatusPort{Writer,Reader}` just `StatusReader` and `StatusWriter`?
