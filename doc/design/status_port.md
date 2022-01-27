# StatusPort

## Summary and problem description

* Taco ähnliches Verhalten mit zwei Speicherzellen
* Auf Daten über ein Lambda zugreifen
    * Lambda nochmal ausführen, wenn sich die Daten unterdrunter geändert haben
      (Frankenstein check)

## Terminology

* Transaction: A user would like to change the world aka the data
* Acknowledged transaction array: Data that made represent the current state of the world aka data
* 

## Design

### Requirements

* Rarely updated
* Many clients are interested in the data
* Access data via lambda to prevent torn reads

### Contract

* 1:N
* Data exchanged needs to be trivially-copyable
* Every `StatusPort`
* No discovery, no `CaPro`, no QoS
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
      of the `StatusPortData`
* Users can only acquire `StatusPortReader`
* Only RouDi is allowed to acquire `StatusPortWriter`

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
