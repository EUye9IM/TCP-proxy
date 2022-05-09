#include <agps/agps.h>
#include <cstdio>
#include <cstring>
namespace agps {
Argument::Argument() {
	set(nullptr, Type::FLAG, 0, nullptr, nullptr, false, Value{0}, nullptr,
		false, Value{0});
}
Argument::Argument(Argument *next_argument, Type type, char short_name,
				   const char *long_name, const char *infomation,
				   bool is_necessary, const Value &default_value,
				   bool (*check_func)(Value), bool is_exist,
				   const Value &real_value) {
	set(next_argument, type, short_name, long_name, infomation, is_necessary,
		default_value, check_func, is_exist, real_value);
}
void Argument::set(Argument *next_argument, Type type, char short_name,
				   const char *long_name, const char *infomation,
				   bool is_necessary, const Value &default_value,
				   bool (*check_func)(Value), bool is_exist,
				   const Value &real_value) {
	this->type = type;
	this->short_name = short_name;
	this->long_name.set(long_name);
	this->infomation.set(infomation);
	this->is_necessary = is_necessary;
	this->default_value = default_value;
	this->check_func = check_func;
	this->next_argument = next_argument;
	this->is_exist = is_exist;
	this->real_value = real_value;
}
Parser::Parser() {
	_b_sucess = false;
	_argument_list = nullptr;
	_raw_list = nullptr;
	_rest_index = nullptr;
	_rest_num = 0;
}
Parser::~Parser() { reset(); }
void Parser::add(Type type, char short_name, const char *long_name,
				 const char *infomation, bool is_necessary,
				 const Value &default_value, bool (*check_func)(Value)) {
	if (short_name == 0 && (long_name == nullptr || long_name[0] == 0))
		return;
	Argument *p_new_arg;
	if (Type::FLAG == type)
		p_new_arg =
			new Argument(nullptr, type, short_name, long_name, infomation,
						 false, VALUE_NONE, nullptr, false, VALUE_NONE);
	else
		p_new_arg = new Argument(nullptr, type, short_name, long_name,
								 infomation, is_necessary, default_value,
								 check_func, false, default_value);
	if (_argument_list == nullptr)
		_argument_list = p_new_arg;
	else {
		Argument *pointer = _argument_list;
		while (pointer->next_argument != nullptr) {
			pointer = pointer->next_argument;
		}
		pointer->next_argument = p_new_arg;
	}
	return;
}
bool static _checkIfArgumentIsMatchRule(const char *arg, const Argument *rule) {
	if (strlen(arg) < 2 || arg[0] != '-')
		return false;
	if (0 != rule->short_name && arg[1] == rule->short_name && 0 == arg[2])
		return true;
	if (0 != rule->short_name && arg[1] == rule->short_name &&
		rule->type != Type::FLAG)
		return true;
	if ('-' == arg[1] && nullptr != rule->long_name.cstr()) {
		if (!strcmp(arg + 2, rule->long_name.cstr()))
			return true;
	}
	return false;
}
void Parser::parse(int argc, const char **argv) {
	_b_sucess = true;
	int pos = 1;
	// save the args
	_raw_list = new ConstStr[argc];
	_rest_index = new int[argc];
	_rest_num = 0;
	for (int i = 0; i < argc; i++)
		_raw_list[i].set(argv[i]);
	// parse arguments
	while (pos < argc && _b_sucess) {
		const char *the_arg = _raw_list[pos].cstr();
		bool is_in_rest_set = true;
		// check if it is -*
		if (strlen(the_arg) >= 2 && the_arg[0] == '-') {
			// find the rule
			Argument *p_rule = _argument_list;
			while (p_rule != nullptr && _b_sucess) {
				if (_checkIfArgumentIsMatchRule(the_arg, p_rule)) {
					p_rule->is_exist = true;
					if (p_rule->type == Type::FLAG) {
						p_rule->real_value.Exist = true;
					} else {
						const char *val_str;
						if (0 != p_rule->short_name &&
							the_arg[1] == p_rule->short_name &&
							0 != the_arg[2]) {
							val_str = the_arg + 2;
						} else {
							pos++;
							if (pos >= argc) {
								_b_sucess = false;
								break;
							}
							val_str = _raw_list[pos].cstr();
						}
						if (val_str[0] == '-')
							_b_sucess = false;
						switch (p_rule->type) {
						case Type::INT:
							int ret, val;
							char c;
							ret = sscanf(val_str, "%d%c", &val, &c);
							if (1 != ret)
								_b_sucess = false;
							else
								p_rule->real_value.Int = val;
							break;

						case Type::STR:
							p_rule->real_value.Str = val_str;
							break;

						case Type::FLAG:
						default:
							_b_sucess = false;
							break;
						} // switch rule type
					}
					is_in_rest_set = false;
					break;
				}
				p_rule = p_rule->next_argument;
			} // rule_list loop : to find which rule to match
			  // match faild
			if (is_in_rest_set) {
				_b_sucess = false;
				break;
			}
		}					  // check if arg[pos] like -*
		if (is_in_rest_set) { // match failed
			_rest_index[_rest_num] = pos;
			_rest_num++;
		}
		// success
		pos++;
	} // arg_list loop

	// check all argument is ok
	Argument *p_rule = _argument_list;
	while (p_rule != nullptr && _b_sucess) {
		if (p_rule->is_necessary && p_rule->is_exist == false)
			_b_sucess = false;
		if (p_rule->check_func != nullptr &&
			!(p_rule->check_func(p_rule->real_value)))
			_b_sucess = false;
		p_rule = p_rule->next_argument;
	}
}
bool Parser::success() const { return _b_sucess; }
Value Parser::get(char short_name) const {
	Argument *pointer = _argument_list;
	while (pointer != nullptr) {
		if (pointer->short_name == short_name)
			return pointer->real_value;
		pointer = pointer->next_argument;
	}
	return VALUE_NONE;
}
Value Parser::get(const char *long_name) const {
	Argument *pointer = _argument_list;
	while (pointer != nullptr) {
		if (pointer->long_name.cstr() != nullptr &&
			!strcmp(pointer->long_name.cstr(), long_name))
			return pointer->real_value;
		pointer = pointer->next_argument;
	}
	return VALUE_NONE;
}
bool Parser::isExist(char short_name) const {
	Argument *pointer = _argument_list;
	while (pointer != nullptr) {
		if (pointer->short_name == short_name)
			return pointer->is_exist;
		pointer = pointer->next_argument;
	}
	return false;
}
bool Parser::isExist(const char *long_name) const {
	Argument *pointer = _argument_list;
	while (pointer != nullptr) {
		if (!strcmp(pointer->long_name.cstr(), long_name))
			return pointer->is_exist;
		pointer = pointer->next_argument;
	}
	return false;
}
const char *Parser::rest(int index) {
	if (index < _rest_num && index >= 0)
		return _raw_list[_rest_index[index]].cstr();
	return nullptr;
}
int Parser::restCnt() { return _rest_num; }
void Parser::printUsage(const char *name, FILE *out_stream) const {
	Argument *pointer;
	fprintf(out_stream, "ussage : %s", name == nullptr ? "" : name);
	pointer = _argument_list;
	while (pointer != nullptr) {
		if (pointer->is_necessary) {
			if (nullptr != pointer->long_name.cstr()) {
				switch (pointer->type) {
				case Type::STR:
					fprintf(out_stream, " --%s {string}",
							pointer->long_name.cstr());
					break;
				case Type::INT:
					fprintf(out_stream, " --%s {integer}",
							pointer->long_name.cstr());
					break;

				case Type::FLAG:
				default:
					fprintf(out_stream, " --%s", pointer->long_name.cstr());
					break;
				}
			} else {
				switch (pointer->type) {
				case Type::STR:
					fprintf(out_stream, " -%c {string}", pointer->short_name);
					break;
				case Type::INT:
					fprintf(out_stream, " -%c {integer}", pointer->short_name);
					break;

				case Type::FLAG:
				default:
					fprintf(out_stream, " -%c", pointer->short_name);
					break;
				}
			}
		}
		pointer = pointer->next_argument;
	}
	fprintf(out_stream, " [options] ...\noptions :\n");
	pointer = _argument_list;
	while (pointer != nullptr) {
		const char *val_type = "";
		int n;

		switch (pointer->type) {
		case Type::STR:
			val_type = "  {string}";
			break;
		case Type::INT:
			val_type = "  {integer}";
			break;
		default:
			break;
		}

		if (nullptr != pointer->long_name.cstr() &&
			pointer->long_name.cstr()[0] != 0) {
			if (0 != pointer->short_name)
				fprintf(out_stream, "    -%c,--%s%s%n", pointer->short_name,
						pointer->long_name.cstr(), val_type, &n);
			else
				fprintf(out_stream, "       --%s%s%n",
						pointer->long_name.cstr(), val_type, &n);
		} else {
			if (0 != pointer->short_name)
				fprintf(out_stream, "    -%c %s%n", pointer->short_name,
						val_type, &n);
			else
				fprintf(out_stream, "     %s%n", val_type, &n);
		}

		if (nullptr != pointer->infomation.cstr() &&
			pointer->infomation.cstr()[0] != 0) {
			if (n < 24) {
				for (int i = n; i < 26; i++) {
					fprintf(out_stream, " ");
				}
			} else {
				fprintf(out_stream, "\n");
				for (int i = 0; i < 26; i++) {
					fprintf(out_stream, " ");
				}
			}
			fprintf(out_stream, "%s", pointer->infomation.cstr());
		}
		fprintf(out_stream, "\n");
		pointer = pointer->next_argument;
	}
	return;
}
void Parser::reset() {
	Argument *head = _argument_list;
	if (_raw_list != nullptr)
		delete[] _raw_list;
	if (_rest_index != nullptr)
		delete[] _rest_index;
	while (head != nullptr) {
		_argument_list = head->next_argument;
		delete head;
		head = _argument_list;
	}
	_b_sucess = false;
	_raw_list = nullptr;
	_rest_index = nullptr;
	_rest_num = 0;
	return;
}
void Parser::clean() {
	Argument *pointer = _argument_list;
	if (_raw_list != nullptr)
		delete[] _raw_list;
	if (_rest_index != nullptr)
		delete[] _rest_index;
	while (pointer != nullptr) {
		pointer->is_exist = false;
		pointer->real_value = pointer->default_value;
		pointer = pointer->next_argument;
	}
	_b_sucess = false;
	_raw_list = nullptr;
	_rest_index = nullptr;
	_rest_num = 0;
	return;
}
} // namespace agps