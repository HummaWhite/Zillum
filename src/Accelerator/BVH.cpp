#include "../../include/Core/BVH.h"

using RadixSortElement = std::pair<int, int>;

struct BoxRec
{
	AABB vertBox;
	AABB centExtent;
};

struct Bucket
{
	Bucket() : count(0) {}
	Bucket(const Bucket& a, const Bucket& b) :
		count(a.count + b.count), box(a.box, b.box) {}
	int count;
	AABB box;
};

struct BuildRec
{
	int offset;
	AABB nodeExtent;
	int splitDim;
	int lRange;
	int rRange;
};

using RadixSortElement = std::pair<int, int>;

void radixSortLH(RadixSortElement* a, int count)
{
	RadixSortElement* b = new RadixSortElement[count];
	int mIndex[4][256];
	memset(mIndex, 0, sizeof(mIndex));

	for (int i = 0; i < count; i++)
	{
		int u = a[i].first;
		mIndex[0][uint8_t(u)]++; u >>= 8;
		mIndex[1][uint8_t(u)]++; u >>= 8;
		mIndex[2][uint8_t(u)]++; u >>= 8;
		mIndex[3][uint8_t(u)]++; u >>= 8;
	}

	int m[4] = { 0, 0, 0, 0 };
	for (int i = 0; i < 256; i++)
	{
		int n[4] = { mIndex[0][i], mIndex[1][i], mIndex[2][i], mIndex[3][i] };
		mIndex[0][i] = m[0];
		mIndex[1][i] = m[1];
		mIndex[2][i] = m[2];
		mIndex[3][i] = m[3];
		m[0] += n[0];
		m[1] += n[1];
		m[2] += n[2];
		m[3] += n[3];
	}

	for (int j = 0; j < 4; j++)
	{
		for (int i = 0; i < count; i++)
		{
			int u = a[i].first;
			b[mIndex[j][uint8_t(u >> (j << 3))]++] = a[i];
		}
		std::swap(a, b);
	}
	delete[] b;
}

void radixSort16(HittableInfo* a, int count, int dim)
{
	RadixSortElement* b = new RadixSortElement[count];

	int l = 0, r = count;
	for (int i = 0; i < count; i++)
	{
		int u = *reinterpret_cast<int*>(&a[i].centroid[dim]);
		if (u & 0x80000000) b[l++] = RadixSortElement(~u, i);
		else b[--r] = RadixSortElement(u, i);
	}
	radixSortLH(b, l);
	radixSortLH(b + l, count - l);

	auto c = new HittableInfo[count];
	memcpy(c, a, count * sizeof(HittableInfo));

	for (int i = 0; i < count; i++)
	{
		a[i] = c[b[i].second];
	}
	delete[] b;
	delete[] c;
}

template<int NumBuckets>
HittableInfo* partition(HittableInfo* a, int size, float axisMin, float axisMax, int splitDim, int splitPoint)
{
	auto tmp = new HittableInfo[size];
	std::copy(a, a + size, tmp);
	int l = 0;
	int r = size;
	for (int i = 0; i < size; i++)
	{
		int b = NumBuckets * (tmp[i].centroid[splitDim] - axisMin) / (axisMax - axisMin);
		b = std::max(std::min(b, NumBuckets - 1), 0);
		(b <= splitPoint ? a[l++] : a[--r]) = tmp[i];
	}
	delete[] tmp;
	if (r == size)
		r--;
	return a + r - 1;
}

BVH::BVH(const std::vector<HittablePtr> &hittables, BVHSplitMethod method) :
    mSplitMethod(method)
{
    if (hittables.size() == 0)
        return;
    std::vector<HittableInfo> hittableInfo;

    AABB rootBox, rootCentExtent;
	for (const auto &hittable : hittables)
	{
		auto box = hittable->bound();
		rootBox.expand(box);
		rootCentExtent.expand(box.centroid());
		hittableInfo.push_back({ box, box.centroid(), hittable });
	}
    mTreeSize = hittables.size() * 2 - 1;
	mTree.resize(mTreeSize);
	//standardBuild(hittableInfo, rootBox);
	quickBuild(hittableInfo, rootCentExtent);
    buildHitTable();
}

bool BVH::testIntersec(const Ray &ray, float dist)
{
    if (mTreeSize == 0)
        return false;
    auto &table = mHitTables[Math::cubeMapFace(-ray.dir)];

    int k = 0;
    while (k != mTreeSize)
    {
        auto [box, hittable, size] = mTree[table[k].nodeIndex];
        auto [boxHit, tMin, tMax] = box.hit(ray);

        if (!boxHit || (boxHit && tMin > dist))
        {
            k = table[k].misNext;
            continue;
        }

        if (hittable != nullptr)
        {
            auto t = hittable->closestHit(ray);
            if (t.has_value())
            {
                if (t.value() < dist)
                    return true;
            }
        }
        k++;
    }
    return false;
}

std::pair<float, HittablePtr> BVH::closestHit(const Ray &ray)
{
    if (mTreeSize == 0)
        return {0.0f, nullptr};
    float dist = 1e8f;
    std::shared_ptr<Hittable> hit;
    auto &table = mHitTables[Math::cubeMapFace(-ray.dir)];

    int k = 0;
    while (k != mTreeSize)
    {
        auto [box, hittable, size] = mTree[table[k].nodeIndex];
        auto [boxHit, tMin, tMax] = box.hit(ray);

        if (!boxHit || (boxHit && tMin > dist))
        {
            k = table[k].misNext;
            continue;
        }

        if (hittable != nullptr)
        {
            auto t = hittable->closestHit(ray);
            if (t.has_value())
            {
                if (t.value() < dist)
                {
                    dist = t.value();
                    hit = hittable;
                }
            }
        }
        k++;
    }
    return {dist, hit};
}

void BVH::quickBuild(std::vector<HittableInfo> &primInfo, const AABB &rootExtent)
{
    std::stack<BuildRec> stack;
	stack.push({ 0, rootExtent, rootExtent.maxExtent(), 0, static_cast<int>(primInfo.size()) - 1 });

	constexpr int NumBuckets = 16;
	int depth = 0;
	while (!stack.empty())
	{
		mDepth = std::max(mDepth, depth);
		auto [offset, nodeExtent, splitDim, l, r] = stack.top();
		stack.pop();
		int size = (r - l) * 2 + 1;
		auto &node = mTree[offset];
        node.size = size;
        node.hittable = (size == 1) ? primInfo[l].hittable : nullptr;

		if (l == r)
		{
			node.bound = primInfo[l].bound;
			depth--;
			continue;
		}
		depth++;
		int nBoxes = r - l + 1;
		if (nBoxes == 2)
		{
			node.bound = AABB(primInfo[l].bound, primInfo[r].bound);
			if (primInfo[l].centroid[splitDim] > primInfo[r].centroid[splitDim])
				std::swap(primInfo[l], primInfo[r]);
			stack.push({ offset + 2, primInfo[r].centroid, 0, r, r });
			stack.push({ offset + 1, primInfo[l].centroid, 0, l, l });
			continue;
		}

		float axisMin = nodeExtent.pMin[splitDim];
		float axisMax = nodeExtent.pMax[splitDim];

		Bucket buckets[NumBuckets];
		Bucket prefix[NumBuckets];
		Bucket suffix[NumBuckets];

		for (int i = l; i <= r; i++)
		{
			int b = NumBuckets * (primInfo[i].centroid[splitDim] - axisMin) / (axisMax - axisMin);
			b = std::max(std::min(b, NumBuckets - 1), 0);
			buckets[b].count++;
			buckets[b].box.expand(primInfo[i].bound);
		}

		prefix[0] = buckets[0];
		suffix[NumBuckets - 1] = buckets[NumBuckets - 1];
		for (int i = 1; i < NumBuckets; i++)
		{
			prefix[i] = Bucket(prefix[i - 1], buckets[i]);
			suffix[NumBuckets - 1 - i] = Bucket(suffix[NumBuckets - i], buckets[NumBuckets - i - 1]);
		}
		node.bound = prefix[NumBuckets - 1].box;

		int splitPoint = 0;
		float minCost = prefix[0].count * prefix[0].box.surfaceArea() +
			suffix[1].count * suffix[1].box.surfaceArea();
		for (int i = 1; i < NumBuckets - 1; i++)
		{
			float cost = prefix[i].count * prefix[i].box.surfaceArea() +
				suffix[i + 1].count * suffix[i + 1].box.surfaceArea();
			if (cost < minCost)
			{
				minCost = cost;
				splitPoint = i;
			}
		}
		auto itr = partition<NumBuckets>(&primInfo[l], nBoxes, axisMin, axisMax, splitDim, splitPoint);
		splitPoint = itr - &primInfo[0];

		AABB lchCentBox, rchCentBox;
		for (int i = l; i <= splitPoint; i++)
			lchCentBox.expand(primInfo[i].centroid);
		for (int i = splitPoint + 1; i <= r; i++)
			rchCentBox.expand(primInfo[i].centroid);

		stack.push({ offset + 2 * (splitPoint - l) + 2, rchCentBox, rchCentBox.maxExtent(), splitPoint + 1, r });
		stack.push({ offset + 1, lchCentBox, lchCentBox.maxExtent(), l, splitPoint });
	}
}

void BVH::standardBuild(std::vector<HittableInfo> &primInfo, const AABB &rootExtent)
{
	// TODO: fix craching bug here.
	std::stack<BuildRec> stack;
	stack.push({ 0, rootExtent, rootExtent.maxExtent(), 0, static_cast<int>(primInfo.size()) - 1 });

	auto prefixes = new BoxRec[primInfo.size()];
	auto suffixes = new BoxRec[primInfo.size()];

	while (!stack.empty())
	{
		auto [offset, nodeExtent, splitDim, l, r] = stack.top();
		stack.pop();
		int size = (r - l) * 2 + 1;
        auto &node = mTree[offset];
        node.size = size;
        node.hittable = (size == 1) ? primInfo[l].hittable : nullptr;

		if (l == r)
		{
            node.bound = primInfo[l].bound;
			continue;
		}
		int nBoxes = r - l + 1;
		radixSort16(primInfo.data() + l, nBoxes, splitDim);
		if (nBoxes == 2)
		{
			node.bound = AABB(primInfo[l].bound, primInfo[r].bound);
			stack.push({ offset + 2, primInfo[r].bound, 0, r, r });
			stack.push({ offset + 1, primInfo[l].bound, 0, l, l });
			continue;
		}
		
		auto prefix = prefixes + l;
		auto suffix = suffixes + l;
		prefix[0] = { primInfo[l].bound, primInfo[l].centroid };
		suffix[nBoxes - 1] = { primInfo[r].bound, primInfo[r].centroid };

		for (int i = 1; i < nBoxes; i++)
		{
			prefix[i] = { AABB(prefix[i - 1].vertBox, primInfo[l + i].bound),
				AABB(prefix[i - 1].centExtent, primInfo[l + i].centroid) };
			suffix[nBoxes - i - 1] = { AABB(suffix[nBoxes - i].vertBox, primInfo[r - i].bound),
				AABB(suffix[nBoxes - i].centExtent, primInfo[r - i].centroid) };
		}
		node.bound = prefix[nBoxes - 1].vertBox;

		auto getCost = [&](int i) -> float
		{
			return prefix[i].vertBox.surfaceArea() * (i + 1) +
				suffix[i + 1].vertBox.surfaceArea() * (nBoxes - i - 1);
		};

		int splitPoint = l;
		float minCost = getCost(0);
		for (int i = 1; i < nBoxes - 1; i++)
		{
			float cost = getCost(i);
			if (cost < minCost)
			{
				minCost = cost;
				splitPoint = l + i;
			}
		}
		AABB lchCentBox = prefix[splitPoint - l].vertBox;
		AABB rchCentBox = suffix[splitPoint - l + 1].vertBox;

		stack.push({ offset + 2 * (splitPoint - l) + 2, rchCentBox, rchCentBox.maxExtent(), splitPoint + 1, r });
		stack.push({ offset + 1, lchCentBox, lchCentBox.maxExtent(), l, splitPoint });
	}
	delete[] prefixes;
	delete[] suffixes;
}

void BVH::buildHitTable()
{
    bool (*cmpFuncs[6])(const glm::vec3 & a, const glm::vec3 & b) =
	{
		[](const glm::vec3& a, const glm::vec3& b) { return a.x > b.x; },	// X+
		[](const glm::vec3& a, const glm::vec3& b) { return a.x < b.x; },	// X-
		[](const glm::vec3& a, const glm::vec3& b) { return a.y > b.y; },	// Y+
		[](const glm::vec3& a, const glm::vec3& b) { return a.y < b.y; },	// Y-
		[](const glm::vec3& a, const glm::vec3& b) { return a.z > b.z; },	// Z+
		[](const glm::vec3& a, const glm::vec3& b) { return a.z < b.z; }	// Z-
	};

	auto stack = new int[mTreeSize];
	for (int i = 0; i < 6; i++)
	{
		auto &table = mHitTables[i];
		table.resize(mTreeSize);
		size_t top = 0;
		int index = 0;
		stack[top++] = 0;

		while (top)
		{
			int nodeIndex = stack[--top];
			auto nodeSize = mTree[nodeIndex].size;
			table[index] = { index + nodeSize, nodeIndex };
			index++;

			if (nodeSize == 1)
				continue;

			int lSize = mTree[nodeIndex + 1].size;
			int lch = nodeIndex + 1;
			int rch = nodeIndex + 1 + lSize;

			if (!cmpFuncs[i](mTree[lch].bound.centroid(), mTree[rch].bound.centroid()))
				std::swap(lch, rch);
			stack[top++] = rch;
			stack[top++] = lch;
		}
	}
	delete[] stack;
}