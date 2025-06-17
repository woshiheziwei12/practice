// 给一个数组nums，和一个正整数 k   你可以将数组中任意数*2操作k次，操作过后的数组的最大或值和。
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
using namespace std;


class Solution2680 {
public:
    using ll = long long;
    long long maximumOr(vector<int>& nums, int k) {
        int n = nums.size();
        vector<ll> suf(n+1,0);

        for (int i = n-1;i>=0;i--) {
            suf[i]= suf[i+1] | nums[i];
        }
        ll res = 0;
        ll pre = 0;
        for (int i = 0; i< n;i++) {
            res = max(res, pre | ((long long)nums[i] << k) | suf[i+1]);
            pre |= nums[i];
        }
        return res;
    }
};

// 快速排序

void qSortArray(int array[], int start, int last)
{
	int low = start;
	int high = last;
	if (low < high)
	{
		while (low < high)
		{
			while (array[low] <= array[start] && low < last)
			{
				low++;//满足小于基准的条件，指针右移
			}
			while (array[high] >= array[start] && high > start)
			{
				high--;//满足大于基准的条件，指针左移
			}
			if (low < high)
			{
				swap(array[low], array[high]);//交换两个不满足条件的元素
			}
			else
			{
				break;
			}
		}
		swap(array[start], array[high]);//插入基准元素
		qSortArray(array, start, high - 1);
		qSortArray(array, high + 1, last);
	}



}

int maxDifference(string s) {
    unordered_map<char, int> wordNum;
	for(char a : s) {
		wordNum[a]++;
	}
	int maxOdd = 1;
	int minEven = s.size();
	for (auto &[_,value]: wordNum){
		if(value%2 == 0 && value < minEven) {
			minEven = value;
		} else if(value%2 == 1 && value > maxOdd) {
			maxOdd = value;
		}
	}
	return maxOdd - minEven;
}



int main(){
	string s = "aaaaabbc";
	cout<<maxDifference(s);
	cout<<"hello";
	return 0;
}
