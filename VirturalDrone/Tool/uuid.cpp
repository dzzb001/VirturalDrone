
#include <stdio.h>
#include <uuid/uuid.h>

int main()
{
	uuid_t uuid;

	char str[36];

	uuid_generate(uuid);
	uuid_unparse(uuid, str);

	printf("%s\n", str);
	return 0;
}