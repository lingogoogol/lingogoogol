#include <iostream>

void sort(int v[], int n) {
    for (int i=0;i<n;++i) {
        for (int j=i-1;j>=0&&v[j]>v[j+1];--j) {
            int temp=v[j];
            v[j]=v[j+1];
            v[j+1]=temp;
        }
    }
    return;
}

auto main() -> int {
    int a[]{ 4,2,3,1 };
    sort(a,4);
    for (int i=0;i<4;++i) {
        std::cout << a[i];
    }
    return 0;
}