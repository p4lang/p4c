#ifndef _KVTABLE_P4_
#define _KVTABLE_P4_

/*
 * This is a target-independent P4 library that provides an abstraction
 * of two table-like objects.
 * Such tables are key-value mappings (like a HashMap).  They differ
 * in the way they handle missing keys (which do not "hit" in the table).
 * A compiler can transform all operations on such tables into
 * operations on standard P4 tables.
 */

/// A type that has one single possible value: the empty tuple.
typedef tuple<> Unit;

/// An Either type has one of two possible values:
/// - the left value
/// - the right value
union Either<LV, RV> {
    LV left;
    RV right;
}

/// An Option type is either empty (containing a 'none' value)
/// or it has some value of type T.
union Option<T> {
    T    some;
    Unit none;
}

/**
 * Table with a match kind M, a key type K, and a value type V.
 * @param K  expected to be struct type
 * @param M  each field of the struct M must be of type match_kind.
 *           should have the same number of fields as K.
 */
extern SimpleTable<M, K, V> {
    /**
     * Create a table with match kinds T and the specified size (number of entries).
     */
    SimpleTable(M match_kinds_tuple, int size);
    /**
     * Lookup the key in the table.  Returns an option that contains the value on
     * hit or none on miss.
     */
    Option<V> lookup(in K key);
    /**
     * Update the entry corresponding to key K (or insert it if it does not exist).
     * Not all targets may support update; programs should give compile-time errors
     * in this case.
     */
    void update(in K key, in V value);
    /**
     * Delete the entry corresponding to key K.
     * Not all targets may support delete; programs should give compile-time errors
     * in this case.
     */
    void delete(in K key);
}

/**
 * Table with match kind M, key type K and two kinds of values:
 * @param HV  type of value returned on a hit
 * @param MV  type of value returned on a miss
 * @param K   type of keys used to index in table; expected to be a struct
 * @param M   should be a struct with as many fields as K; each field of M
 *            must be of type match_kind.
 * The table maps each key to a value of type MV.
 * All keys that are not present in the table map to the same default value of type MV.
 */
extern Table<M, K, HV, MV> {
    /**
     * Create a table with match kinds M and the specified size (number of entries)
     */
    Table(M match_kinds_tuple, int size);
    /**
     * Lookup the key in the table.  Returns an Either structure, with
     * the left field present on a hit, or the right field on a miss.
     */
    Either<HV, MV> lookup(in K key);
    /**
     * Update the entry corresponding to hit on key K (or insert it if it does not exist).
     * Not all targets may support update; programs should give compile-time errors
     * in this case.
     */
    void update(in K key, in HV value);
    /**
     * Update the entry corresponding to a miss.
     * Not all targets may support updateMiss; programs should give compile-time errors
     * in this case.
     */
    void updateMiss(in MV value);
    /**
     * Delete the entry corresponding to key K.
     * Not all targets may support delete; programs should give compile-time errors
     * in this case.
     */
    void delete(in K key);
}

#endif // _KVTABLE_P4_