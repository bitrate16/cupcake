#pragma once

#include "vobject.h"
#include "vscope.h"

// U N U S E D

// Object created by prototype::call
template<typename ProtoClass, typename std::enable_if<std::is_base_of<vobject, ProtoClass>::value>::type* = nullptr>
class ProtoObject<ProtoClass> : public vobject {
	
	protected:
	
	std::map<std::wstring, ck_vobject::vobject*> objects;
	
	public:
	
	ProtoObject();
	
	// + default
	virtual vobject* get     (vscope*, const std::wstring&);
	virtual void     put     (vscope*, const std::wstring&, vobject*);
	virtual bool     contains(vscope*, const std::wstring&);
	virtual bool     remove  (vscope*, const std::wstring&);
	virtual vobject* call    (vscope*, const std::vector<vobject*>&);
	
	virtual void gc_mark();
	virtual void gc_finalize();
	
	virtual int64_t int_value();
	
	virtual std::wstring string_value();
}

// prototype
template<typename ProtoProtoClass, typename std::enable_if<std::is_base_of<vobject, ProtoProtoClass>::value>::type* = nullptr>
class Proto<ProtoProtoClass> : public vobject {
	
	protected:
	
	std::map<std::wstring, ck_vobject::vobject*> objects;
	Proto* self_instance;
	
	public:
	
	Proto();
	
	virtual vobject* get     (vscope*, const std::wstring&);
	virtual void     put     (vscope*, const std::wstring&, vobject*);
	virtual bool     contains(vscope*, const std::wstring&);
	virtual bool     remove  (vscope*, const std::wstring&);
	
	virtual vobject* call    (vscope*, const std::vector<vobject*>&); // <-- only overridable
	
	virtual void gc_mark();
	virtual void gc_finalize();
	
	virtual int64_t int_value();
	
	virtual std::wstring string_value();
	
	virtual static void init_proto();
	
	inline Proto* instance() { return self_instance; };
}

// Example:
// Array : Proto<Object>
// ArrayObject : ProtoObject<Array>