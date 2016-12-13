#ifndef ALLSCALE_WORK_ITEM_DEPENDENCY_HPP
#define ALLSCALE_WORK_ITEM_DEPENDENCY_HPP

namespace allscale
{
    class work_item_dependency 
    {
	public:
           std::string parent;
           std::string child;

           work_item_dependency(std::string p, std::string c) :
		parent(p), child(c) {};

           ~work_item_dependency() {};

    };

}

#endif


