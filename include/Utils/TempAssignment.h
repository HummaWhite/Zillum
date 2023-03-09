#pragma once

template<typename T>
class TempAssignment {
public:
    TempAssignment(T *reference = nullptr, const T &newValue = T()) {
        assign(reference, newValue);
    }

    TempAssignment(T &reference, const T &newValue) {
        assign(&reference, newValue);
    }

    ~TempAssignment() {
        if (mReference)
            *mReference = mBackup;
    }

    TempAssignment& operator = (TempAssignment &&rhs) {
        mReference = rhs.mReference;
        mBackup = rhs.mBackup;
        rhs.mReference = nullptr;
        return *this;
    }

    void assign(T *reference, const T &newValue) {
        if (reference) {
            mBackup = *reference;
            *reference = newValue;
        }
        mReference = reference;
    }

private:
    T *mReference;
    T mBackup;
};