#include "UnrealEnginePythonPrivatePCH.h"
#include "PyCharacter.h"



APyCharacter::APyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	PythonTickForceDisabled = false;
	PythonDisableAutoBinding = false;

	FScopePythonGIL gil;
	// pre-generate PyUObject (for performance)
	ue_get_python_wrapper(this);
}


// Called when the game starts
void APyCharacter::BeginPlay()
{
	

	Super::BeginPlay();

	// ...

	if (PythonModule.IsEmpty())
		return;

	FScopePythonGIL gil;

	PyObject *py_character_module = PyImport_ImportModule(TCHAR_TO_UTF8(*PythonModule));
	if (!py_character_module) {
		unreal_engine_py_log_error();
		return;
	}

#if WITH_EDITOR
	// todo implement autoreload with a dictionary of module timestamps
	py_character_module = PyImport_ReloadModule(py_character_module);
	if (!py_character_module) {
		unreal_engine_py_log_error();
		return;
	}
#endif

	if (PythonClass.IsEmpty())
		return;

	PyObject *py_character_module_dict = PyModule_GetDict(py_character_module);
	PyObject *py_character_class = PyDict_GetItemString(py_character_module_dict, TCHAR_TO_UTF8(*PythonClass));

	if (!py_character_class) {
		unreal_engine_py_log_error();
		return;
	}

	py_character_instance = PyObject_CallObject(py_character_class, NULL);
	if (!py_character_instance) {
		unreal_engine_py_log_error();
		return;
	}

	py_uobject = ue_get_python_wrapper(this);

	if (py_uobject) {
		PyObject_SetAttrString(py_character_instance, "uobject", (PyObject *)py_uobject);
	}
	else {
		UE_LOG(LogPython, Error, TEXT("Unable to set 'uobject' field in character wrapper class"));
		unreal_engine_py_log_error();
		return;
	}

	// disable ticking if no tick method is exposed
	if (!PyObject_HasAttrString(py_character_instance, "tick") || PythonTickForceDisabled) {
		SetActorTickEnabled(false);
	}

	if (!PythonDisableAutoBinding)
		ue_autobind_events_for_pyclass(py_uobject, py_character_instance);

	if (!PyObject_HasAttrString(py_character_instance, "begin_play")) {
		return;
	}

	PyObject *bp_ret = PyObject_CallMethod(py_character_instance, "begin_play", NULL);
	if (!bp_ret) {
		unreal_engine_py_log_error();
		return;
	}
	Py_DECREF(bp_ret);
}


// Called every frame
void APyCharacter::Tick(float DeltaTime)
{
	

	Super::Tick(DeltaTime);

	if (!py_character_instance)
		return;

	FScopePythonGIL gil;

	// no need to check for method availability, we did it in begin_play

	PyObject *ret = PyObject_CallMethod(py_character_instance, "tick", "f", DeltaTime);
	if (!ret) {
		unreal_engine_py_log_error();
		return;
	}
	Py_DECREF(ret);

}

void APyCharacter::CallPyCharacterMethod(FString method_name, FString args)
{
	

	if (!py_character_instance)
		return;

	FScopePythonGIL gil;

	PyObject *ret = nullptr;
	if (args.IsEmpty()) {
		ret = PyObject_CallMethod(py_character_instance, TCHAR_TO_UTF8(*method_name), NULL);
	}
	else {
		ret = PyObject_CallMethod(py_character_instance, TCHAR_TO_UTF8(*method_name), "s", TCHAR_TO_UTF8(*args));
	}

	if (!ret) {
		unreal_engine_py_log_error();
		return;
	}
	Py_DECREF(ret);
}

void APyCharacter::SetPythonAttrObject(FString attr, UObject *object)
{
	

	if (!py_character_instance)
		return;

	FScopePythonGIL gil;

	ue_PyUObject *py_obj = ue_get_python_wrapper(object);
	if (!py_obj) {
		PyErr_Format(PyExc_Exception, "PyUObject is in invalid state");
		unreal_engine_py_log_error();
		return;
	}

	if (PyObject_SetAttrString(py_character_instance, TCHAR_TO_UTF8(*attr), (PyObject *)py_obj) < 0) {
		UE_LOG(LogPython, Error, TEXT("Unable to set attribute %s"), *attr);
	}
}

void APyCharacter::SetPythonAttrString(FString attr, FString s)
{
	

	if (!py_character_instance)
		return;

	FScopePythonGIL gil;

	if (PyObject_SetAttrString(py_character_instance, TCHAR_TO_UTF8(*attr), PyUnicode_FromString(TCHAR_TO_UTF8(*s))) < 0) {
		UE_LOG(LogPython, Error, TEXT("Unable to set attribute %s"), *attr);
	}
}

void APyCharacter::SetPythonAttrFloat(FString attr, float f)
{
	

	if (!py_character_instance)
		return;

	FScopePythonGIL gil;

	if (PyObject_SetAttrString(py_character_instance, TCHAR_TO_UTF8(*attr), PyFloat_FromDouble(f)) < 0) {
		UE_LOG(LogPython, Error, TEXT("Unable to set attribute %s"), *attr);
	}
}

void APyCharacter::SetPythonAttrInt(FString attr, int n)
{
	

	if (!py_character_instance)
		return;

	FScopePythonGIL gil;

	if (PyObject_SetAttrString(py_character_instance, TCHAR_TO_UTF8(*attr), PyLong_FromLong(n)) < 0) {
		UE_LOG(LogPython, Error, TEXT("Unable to set attribute %s"), *attr);
	}
}

void APyCharacter::SetPythonAttrVector(FString attr, FVector vec)
{
	

	if (!py_character_instance)
		return;

	FScopePythonGIL gil;

	if (PyObject_SetAttrString(py_character_instance, TCHAR_TO_UTF8(*attr), py_ue_new_fvector(vec)) < 0) {
		UE_LOG(LogPython, Error, TEXT("Unable to set attribute %s"), *attr);
	}
}

void APyCharacter::SetPythonAttrRotator(FString attr, FRotator rot)
{
	

	if (!py_character_instance)
		return;

	FScopePythonGIL gil;

	if (PyObject_SetAttrString(py_character_instance, TCHAR_TO_UTF8(*attr), py_ue_new_frotator(rot)) < 0) {
		UE_LOG(LogPython, Error, TEXT("Unable to set attribute %s"), *attr);
	}
}

void APyCharacter::SetPythonAttrBool(FString attr, bool b)
{
	

	if (!py_character_instance)
		return;

	FScopePythonGIL gil;
	
	PyObject *py_bool = Py_False;
	if (b) {
		py_bool = Py_True;
	}

	if (PyObject_SetAttrString(py_character_instance, TCHAR_TO_UTF8(*attr), py_bool) < 0) {
		UE_LOG(LogPython, Error, TEXT("Unable to set attribute %s"), *attr);
	}
}

bool APyCharacter::CallPyCharacterMethodBool(FString method_name, FString args)
{
	

	if (!py_character_instance)
		return false;

	FScopePythonGIL gil;

	PyObject *ret = nullptr;
	if (args.IsEmpty()) {
		ret = PyObject_CallMethod(py_character_instance, TCHAR_TO_UTF8(*method_name), NULL);
	}
	else {
		ret = PyObject_CallMethod(py_character_instance, TCHAR_TO_UTF8(*method_name), "s", TCHAR_TO_UTF8(*args));
	}
	

	if (!ret) {
		unreal_engine_py_log_error();
		return false;
	}

	if (PyObject_IsTrue(ret)) {
		Py_DECREF(ret);
		return true;
	}

	Py_DECREF(ret);
	return false;
}

float APyCharacter::CallPyCharacterMethodFloat(FString method_name, FString args)
{
	

	if (!py_character_instance)
		return false;

	FScopePythonGIL gil;

	PyObject *ret = nullptr;
	if (args.IsEmpty()) {
		ret = PyObject_CallMethod(py_character_instance, TCHAR_TO_UTF8(*method_name), NULL);
	}
	else {
		ret = PyObject_CallMethod(py_character_instance, TCHAR_TO_UTF8(*method_name), "s", TCHAR_TO_UTF8(*args));
	}


	if (!ret) {
		unreal_engine_py_log_error();
		return false;
	}

	float value = 0;

	if (PyNumber_Check(ret)) {
		PyObject *py_value = PyNumber_Float(ret);
		value = PyFloat_AsDouble(py_value);
		Py_DECREF(py_value);
	}

	Py_DECREF(ret);
	return value;
}

FString APyCharacter::CallPyCharacterMethodString(FString method_name, FString args)
{
	

	if (!py_character_instance)
		return FString();

	FScopePythonGIL gil;

	PyObject *ret = nullptr;
	if (args.IsEmpty()) {
		ret = PyObject_CallMethod(py_character_instance, TCHAR_TO_UTF8(*method_name), NULL);
	}
	else {
		ret = PyObject_CallMethod(py_character_instance, TCHAR_TO_UTF8(*method_name), "s", TCHAR_TO_UTF8(*args));
	}

	if (!ret) {
		unreal_engine_py_log_error();
		return FString();
	}

	PyObject *py_str = PyObject_Str(ret);
	if (!py_str) {
		Py_DECREF(ret);
		return FString();
	}

	char *str_ret = PyUnicode_AsUTF8(py_str);

	FString ret_fstring = FString(UTF8_TO_TCHAR(str_ret));

	Py_DECREF(py_str);

	return ret_fstring;
}

// Called to bind functionality to input
void APyCharacter::SetupPlayerInputComponent(class UInputComponent* input)
{
	Super::SetupPlayerInputComponent(input);

}


APyCharacter::~APyCharacter()
{
	FScopePythonGIL gil;
#if UEPY_MEMORY_DEBUG
	if (py_character_instance && py_character_instance->ob_refcnt != 1) {
		UE_LOG(LogPython, Error, TEXT("Inconsistent Python ACharacter wrapper refcnt = %d"), py_character_instance->ob_refcnt);
	}
#endif
	Py_XDECREF(py_character_instance);
#if UEPY_MEMORY_DEBUG
	UE_LOG(LogPython, Warning, TEXT("Python ACharacter (mapped to %p) wrapper XDECREF'ed"), py_uobject ? py_uobject->ue_object : nullptr);
#endif
}