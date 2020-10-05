#pragma once
////////////////////////////////////
// 関数形式マクロ
////////////////////////////////////

////////////////////////////////////
// インターフェイス解放
////////////////////////////////////
template <typename T>
inline void safeRelease(T& Ptr) { if (Ptr != nullptr)Ptr->Release(); Ptr = nullptr; }

////////////////////////////////////
// メモリ解放
////////////////////////////////////
#define SAFE_DELETE(x)         {delete x;  x=nullptr;}// 1つ
#define SAFE_DELETE_ARRAY(x)   {delete[] x;x=nullptr;}// 配列
#define SAFE_DELETE_DESTROY(x) {x->destroy();delete x;x = nullptr;}