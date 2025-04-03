#ifndef EXPORTACTIONS_H
#define EXPORTACTIONS_H

namespace AutoQuill {
	class DocumentTemplate;
}
class MainWindows;

bool exportTemplateUsingJson(AutoQuill::DocumentTemplate* documentTemplate,
                             MainWindows* mainWindows);

#endif // EXPORTACTIONS_H
