#include "sf2_resource.h"

Error Sf2Resource::load_file(const String &p_path)
{
    // open resource file
    this->file_buffer = FileAccess::get_file_as_bytes(p_path);
    if (this->file_buffer.size() == 0)
    {
        Error err = FileAccess::get_open_error();
        UtilityFunctions::print("[GodotMidi] Failed to open file: " + p_path + " with error: " + itos(err));

        return FAILED;
    }

    return OK;
}

Error Sf2Resource::save_file(const String &p_path, const Ref<Resource> &p_resource)
{
    return OK;
}