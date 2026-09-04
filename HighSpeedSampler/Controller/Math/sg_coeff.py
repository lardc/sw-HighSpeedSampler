# Recalculate Savitzky-Golay weights for SavitzkyGolay.cpp.
# WINDOW must be odd, POLY < WINDOW. Paste the printed array into SG_COEFF
# and set SG_LENGTH to WINDOW.
#
# python sg_coeff.py

import numpy as np

WINDOW = 21
POLY = 3

assert WINDOW % 2 == 1 and 0 <= POLY < WINDOW

m = WINDOW // 2
t = np.arange(-m, m + 1, dtype=float)
A = np.vander(t, N=POLY + 1, increasing=True)
c = np.linalg.solve(A.T @ A, A.T)[0]

print("#define SG_LENGTH\t%d" % WINDOW)
print("const float SG_COEFF[SG_LENGTH] = {")
print(",\n".join("\t%.8ef" % v for v in c))
print("};")
