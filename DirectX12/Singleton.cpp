#include "stdafx.h"
#include "Singleton.h"
namespace
{
	std::mutex mutex;
	std::vector<SingletonFinalizer::FinalizerFunc> finalizers;
}

void SingletonFinalizer::addFinalizer(FinalizerFunc Func)
{
	std::lock_guard<std::mutex>lock(mutex);
	
	//“o˜^
	finalizers.push_back(Func);
}

void SingletonFinalizer::finalize()
{
	std::lock_guard<std::mutex> lock(mutex);

	//”z—ñƒTƒCƒYŽæ“¾
	unsigned int size = finalizers.size();

	//“o˜^‚µ‚½‹t‡‚Å‰ð•ú
	for (int i = size - 1; i >= 0; i--)
	{
		(*finalizers[i])();
	}

	//—v‘f‚ðƒNƒŠƒA
	finalizers.clear();
}
