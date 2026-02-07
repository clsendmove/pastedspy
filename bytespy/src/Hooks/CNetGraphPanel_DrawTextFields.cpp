#include "../SDK/SDK.h"

#include "../SDK/Vars.h"

MAKE_SIGNATURE(Q_snprintf, "client.dll", "4C 89 44 24 ? 4C 89 4C 24 ? 53 55 56 57 41 56 48 83 EC ? 49 8B D8 48 63 FA 48 8B F1 4C 8D 74 24 ? E8 ? ? ? ? 4C 89 74 24 ? 4C 8B CB 4C 8B C7", 0x0);

MAKE_HOOK(Q_snprintf, S::Q_snprintf(), int,
    char* buffer, int size, const char* format, ...)
{
    if (!format || !buffer)
        return -1;

    va_list args;
    va_start(args, format);

    const char* string = "bytespy v2.5";

    // is this the fps string?
    if (Vars::Visuals::UI::NetGraphWatermark.Value && strcmp(format, "fps:%4i   ping: %i ms") == 0)
    {
        va_arg(args, int); // fps value
        va_arg(args, int); // ping value
        va_end(args);
        strcpy_s(buffer, size, string);

		return (int)strlen(string);
    }

    int result = vsnprintf(buffer, size, format, args);
    va_end(args);

    return result;
}