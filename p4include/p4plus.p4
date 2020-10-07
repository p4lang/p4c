/* P4 extensions for P4+ */

typedef Either<L, R> = Left { L value }
                     | Right { R value }

extern Relation<K, VH, VM> {
    Relation();
    Either<VH, VM> lookup(K key);
}

extern SimpleRelation<K, V> {
    SimpleRelation();
    V lookup(K key);
    void update(K key, V value);
}