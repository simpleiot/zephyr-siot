#ifndef HTML_H_
#define HTML_H_

typedef void (*html_form_callback)(char *key, char *value);

void html_parse_form_data(const char *body, html_form_callback callback);

#endif // HTML_H_
