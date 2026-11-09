import sys
from pathlib import Path
import unittest
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from evaluate_stemgenrt_quality import si_sdr

class QualityOracle(unittest.TestCase):
    def test_known_projection_and_orthogonal_error(self):
        reference=np.array([1.,-1.,1.,-1.])
        estimate=2*reference+np.array([1.,1.,-1.,-1.])
        self.assertAlmostEqual(si_sdr(reference,estimate),6.020599913279624)

    def test_silent_reference_is_not_a_quality_score(self):
        self.assertIsNone(si_sdr(np.zeros(4),np.ones(4)))

    def test_length_mismatch_and_nonfinite_fail(self):
        with self.assertRaises(ValueError):si_sdr(np.ones(4),np.ones(3))
        with self.assertRaises(ValueError):si_sdr(np.array([1.,2.,np.nan]),np.ones(3))

if __name__=="__main__":unittest.main()
