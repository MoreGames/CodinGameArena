#ifndef RANDOM_H
#define RANDOM_H

// a copy of the Java Random class

class Random {
private:
	long long m_seed;
	inline void setSeed(long long seed) {
		m_seed = (seed ^ 0x5DEECE66DL) & ((1LL << 48) - 1);
	}
	inline int next(int bits) {
		m_seed = (m_seed * 0x5DEECE66DL + 0xBL) & ((1LL << 48) - 1);
		return (int)(m_seed >> (48 - bits));
	}
public:
	Random(long long seed) {
		setSeed(seed);
	}

	long long getSeed() {
		return m_seed;
	}
	int nextInt() {
		return next(32);
	}
	int nextInt(int n) {
		if ((n & (n - 1)) == 0) // i.e., n is a power of 2
			return (int)((n * (long long)next(31)) >> 31);
		int bits, val;
		do {
			bits = next(31);
			val = bits % n;
		} while (bits - val + (n - 1) < 0);
		return val;
	}
	long long nextLong() {
		// it's okay that the bottom word remains signed.
		return ((long long)(next(32)) << 32) + next(32);
	}
};

#endif