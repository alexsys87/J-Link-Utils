#include "Preferences.h"
void WriteFromOtherTranslationUnit() { prefs::WriteString("shared.key", "one process-wide store"); }
