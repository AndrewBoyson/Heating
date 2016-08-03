#define HTTP_DATE_SIZE 30

extern void HttpMakeDate(const char* date, const char *time, char* text);
extern void HttpOk         (char *pContentType, char *pLastModified, char *pCacheControl);
extern void HttpNotModified(char *pContentType, char *pLastModified, char *pCacheControl);
extern void HttpNotFound();
extern void HttpBadRequest();
extern void HttpBadMethod();
extern void HttpNotImplemented();
