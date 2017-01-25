#-----------------------------------------------------------------------------#
# Sample UTS Workloads:
#
#  This file contains sample workloads for UTS, along with the tree statistics
#  for verifying correct output from the benchmark.  This file is intended to
#  be used in shell scripts or from the shell so that UTS can be run by:
#
#   $ source sample_workloads.csh
#   $ ./uts $T1
#
#-----------------------------------------------------------------------------#

# ====================================
# Small Workloads (~4 million nodes):
# ====================================

# (T1) Geometric [fixed] ------- Tree size = 4130071, tree depth = 10, num leaves = 3305118 (80.03%)
setenv T1 "--tree-type 1 --tree-shape 3 --tree-depth 10 --root-branching-factor 4 --root-seed 19"

# (T5) Geometric [linear dec.] - Tree size = 4147582, tree depth = 20, num leaves = 2181318 (52.59%)
setenv T5 "--tree-type 1 --tree-shape 0 --tree-depth 20 --root-branching-factor 4 --root-seed 34"

# (T2) Geometric [cyclic] ------ Tree size = 4117769, tree depth = 81, num leaves = 2342762 (56.89%)
setenv T2 "--tree-type 1 --tree-shape 2 --tree-depth 16 --root-branching-factor 6 --root-seed 502"

# (T3) Binomial ---------------- Tree size = 4112897, tree depth = 1572, num leaves = 3599034 (87.51%)
setenv T3 "--tree-type 0 --root-branching-factor 2000 --non-leaf-probability 0.124875 --num-children 8 --root-seed 42"

# (T4) Hybrid ------------------ Tree size = 4132453, tree depth = 134, num leaves = 3108986 (75.23%)
setenv T4 "--tree-type 2 --tree-shape 0 --tree-depth 16 --root-branching-factor 6 --root-seed 1 --non-leaf-probability 0.234375 --num-children 4"

# ====================================
# Large Workloads (~100 million nodes):
# ====================================

# (T1L) Geometric [fixed] ------ Tree size = 102181082, tree depth = 13, num leaves = 81746377 (80.00%)
setenv T1L "--tree-type 1 --tree-shape 3 --tree-depth 13 --root-branching-factor 4 --root-seed 29"

# (T2L) Geometric [cyclic] ----- Tree size = 96793510, tree depth = 67, num leaves = 53791152 (55.57%)
setenv T2L "--tree-type 1 --tree-shape 2 --tree-depth 23 --root-branching-factor 7 --root-seed 220"

# (T3L) Binomial --------------- Tree size = 111345631, tree depth = 17844, num leaves = 89076904 (80.00%)
setenv T3L "--tree-type 0 --root-branching-factor 2000 --non-leaf-probability 0.200014 --num-children 5 --root-seed 7"

# ====================================
# Extra Large (XL) Workloads (~1.6 billion nodes):
# ====================================

# (T1XL) Geometric [fixed] ----- Tree size = 1635119272, tree depth = 15, num leaves = 1308100063 (80.00%)
setenv T1XL "--tree-type 1 --tree-shape 3 --tree-depth 15 --root-branching-factor 4 --root-seed 29"

# ====================================
# Extra Extra Large (XXL) Workloads (~3-10 billion nodes):
# ====================================

# (T1XXL) Geometric [fixed] ---- Tree size = 4230646601, tree depth = 15 
setenv T1XXL "--tree-type 1 --tree-shape 3 --tree-depth 15 --root-branching-factor 4 --root-seed 19"

# (T3XXL) Binomial ------------- Tree size = 2793220501 
setenv T3XXL "--tree-type 0 --root-branching-factor 2000 --non-leaf-probability 0.499995 --num-children 2 --root-seed 316"

# (T2XXL) Binomial ------------- Tree size = 10612052303, tree depth = 216370, num leaves = 5306027151 (50.00%) 
setenv T2XXL "--tree-type 0 --root-branching-factor 2000 --non-leaf-probability 0.499999995 --num-children 2 --root-seed 0"

# ====================================
# Wicked Large Workloads (~150-300 billion nodes):
# ====================================

# (T1WL) Geometric [fixed] ----- Tree size = 270751679750, tree depth = 18, num leaves = 216601257283 (80.00%)
setenv T1WL "--tree-type 1 --tree-shape 3 --tree-depth 18 --root-branching-factor 4 --root-seed 19"

# (T2WL) Binomial -------------- Tree size = 295393891003, tree depth = 1021239, num leaves = 147696946501 (50.00%)
setenv T2WL "--tree-type 0 --root-branching-factor 2000 --non-leaf-probability 0.4999999995 --num-children 2 --root-seed 559"

# (T3WL) Binomial -------------- Tree size = T3WL: Tree size = 157063495159, tree depth = 758577, num leaves = 78531748579 (50.00%) 
setenv T3WL "--tree-type 0 --root-branching-factor 2000 --non-leaf-probability 0.4999995 --num-children 2 --root-seed 559"

