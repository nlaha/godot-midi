#ifndef GODOTMIDI_REGISTER_TYPES_H
#define GODOTMIDI_REGISTER_TYPES_H

// We don't need windows.h in this plugin but many others do and it throws up on itself all the time
// So best to include it and make sure CI warns us when we use something Microsoft took for their own goals....
#ifdef WIN32
#include <windows.h>
#endif

void initialize_godotmidi_types();
void uninitialize_godotmidi_types();

#endif // GODOTMIDI_REGISTER_TYPES_H
