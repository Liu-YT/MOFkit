```
relation = {}
connect_list = []

func find_solvent()
    // build connect_list and inital mapping relation
    for var i = 0 to all_atoms.size() by i++ do
        for var j = i+1 to all_atoms.size() by j++ do
            a = all_atoms[i], b = all_atoms[j]
            if dis(a,b) > 0.5 * (r_a + r_b) and dis(a,b) < 1.2 * (r_a + r_b):
                connect_list.add(Connect(a,b))
            relation[a] = a
            relation[b] = b

    // disjoint set, find atomic correlations
    for connect in connect_list
        if get_ancestor(a) == get_ancestor(b)
            continue
        else if get_ancestor(a) == a
            relation[get_ancestor(b)] = a
        else if get_ancestor(b) = b
            relation[get_ancestor(a)] = b
        else
            relation[get_ancestor(b)] = get_ancestor(a)

    // result set partioning 
    all_subset = {}
    for atom in all_atoms
        ancestor = get_ancestor(atom)
        all_subset[ancestor].add(atom)

    // component identification
    framework = []
    solvent = []
    guest = []
    for res in all_subset
        if res contains carbon and metal atoms
            framework.add(res)
        else if res in solvents_list
            solvent.add(res)
        else 
            guest.add(res)
            

// get real ancestor
func get_ancestor(atom)
    if relation(atom) == atom
        return atom
    return get_ancestor(relation(atom))
```