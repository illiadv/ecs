#include <vector>
template <typename T>
class SparseSet {
public:
    SparseSet<T>(int size)
	: m_size(size)
    {
	sparse.resize(size);
    }

    void Insert(size_t index, T element) {
	if (index >= m_size)
	    return;
	sparse[index] = dense.size();
	dense.push_back(element);
    }

private:
    int m_size;
    std::vector<size_t> sparse;
    std::vector<T> dense;
};
