// Copyright (c) 2024 ACM Class, SJTU

namespace sjtu {

class BuddyAllocator {
  enum State { FREE, ALLOCATED, SPLIT };

  int ram_size;
  int min_block_size;
  int L;
  int num_leaves;
  State* state;
  int* max_free;

  int my_max(int a, int b) {
    return (a > b) ? a : b;
  }

  void update(int node, int level) {
    if (state[node] == FREE) {
      max_free[node] = level;
    } else if (state[node] == ALLOCATED) {
      max_free[node] = -1;
    } else {
      max_free[node] = my_max(max_free[2 * node], max_free[2 * node + 1]);
    }
  }

  int get_k(int size) {
    int k = 0;
    int s = min_block_size;
    while (s < size) {
      s <<= 1;
      k++;
    }
    return k;
  }

  void init_max_free(int node, int level) {
    max_free[node] = level;
    if (level > 0) {
      init_max_free(2 * node, level - 1);
      init_max_free(2 * node + 1, level - 1);
    }
  }

public:
  BuddyAllocator(int ram_size, int min_block_size) : ram_size(ram_size), min_block_size(min_block_size) {
    num_leaves = 1;
    L = 0;
    while (num_leaves * min_block_size < ram_size) {
      num_leaves <<= 1;
      L++;
    }
    int num_nodes = 2 * num_leaves;
    state = new State[num_nodes];
    max_free = new int[num_nodes];
    for (int i = 0; i < num_nodes; ++i) {
      state[i] = FREE;
    }
    init_max_free(1, L);
  }

  ~BuddyAllocator() {
    delete[] state;
    delete[] max_free;
  }

  int malloc(int size) {
    int k = get_k(size);
    if (k > L || max_free[1] < k) return -1;
    return allocate(1, L, k) * min_block_size;
  }

  int allocate(int node, int level, int k) {
    if (level == k && state[node] == FREE) {
      state[node] = ALLOCATED;
      max_free[node] = -1;
      return 0;
    }
    
    if (state[node] == FREE) {
      state[node] = SPLIT;
    }

    int res;
    if (max_free[2 * node] >= k) {
      res = allocate(2 * node, level - 1, k);
    } else {
      res = (1 << (level - 1)) + allocate(2 * node + 1, level - 1, k);
    }
    update(node, level);
    return res;
  }

  int malloc_at(int addr, int size) {
    int k = get_k(size);
    int leaf_idx = addr / min_block_size;
    if (k > L || !can_allocate_at(1, L, leaf_idx, k)) return -1;
    do_allocate_at(1, L, leaf_idx, k);
    return addr;
  }

  bool can_allocate_at(int node, int level, int leaf_idx, int k) {
    if (level == k) {
      return state[node] == FREE;
    }
    if (state[node] == ALLOCATED) return false;
    
    int mid = 1 << (level - 1);
    if (leaf_idx < mid) {
      return can_allocate_at(2 * node, level - 1, leaf_idx, k);
    } else {
      return can_allocate_at(2 * node + 1, level - 1, leaf_idx - mid, k);
    }
  }

  void do_allocate_at(int node, int level, int leaf_idx, int k) {
    if (level == k) {
      state[node] = ALLOCATED;
      max_free[node] = -1;
      return;
    }
    if (state[node] == FREE) {
      state[node] = SPLIT;
    }
    int mid = 1 << (level - 1);
    if (leaf_idx < mid) {
      do_allocate_at(2 * node, level - 1, leaf_idx, k);
    } else {
      do_allocate_at(2 * node + 1, level - 1, leaf_idx - mid, k);
    }
    update(node, level);
  }

  void free_at(int addr, int size) {
    int k = get_k(size);
    int leaf_idx = addr / min_block_size;
    do_free_at(1, L, leaf_idx, k);
  }

  void do_free_at(int node, int level, int leaf_idx, int k) {
    if (level == k) {
      state[node] = FREE;
      max_free[node] = level;
      return;
    }
    int mid = 1 << (level - 1);
    if (leaf_idx < mid) {
      do_free_at(2 * node, level - 1, leaf_idx, k);
    } else {
      do_free_at(2 * node + 1, level - 1, leaf_idx - mid, k);
    }
    
    if (state[2 * node] == FREE && state[2 * node + 1] == FREE) {
      state[node] = FREE;
    } else {
      state[node] = SPLIT;
    }
    update(node, level);
  }
};

} // namespace sjtu