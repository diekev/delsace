#include <atomic>
#include <limits>

class UT_StringRef {
protected:
	class Holder {
		char *_string = nullptr; // pointeur vers une chaine partagée
		size_t _length = 0ul;
		uint _hash = 0;
		std::atomic<int> _refcount = 0;

	public:
		Holder() = default;

		~Holder()
		{
			if (_refcount.fetch() == 0) {
				free(_string);
			}
		}

		const char *string() const
		{
			return _string;
		}

		size_t length() const
		{
			return _length;
		}

		uint hash() const
		{
			return _hash;
		}

		void reference()
		{
			_refcount.fetch_add(1);
		}

		void unreference()
		{
			_refcount.fetch_add(-1);
		}
	};

	union {
		const char *_dataIfChars; // pointeur vers chaine littérale
		Holder *_dataIfHolder;
	};

	uint _length; // agie comme un drapeau pour déterminer si holder ou non
	uint _hash;

public:
	UT_StringRef(const char *str)
	    : UT_StringHolder(str, std::strlen(str))
	{}

	UT_StringRef(const char *str, size_t len)
	{
		if (str && len && len <= std::numeric_limits<unsigned int>::max()) {
			_length = len;
			_hash = computeHash(str); // constexpr
			_dataIfChars = str; // Point to the literal string
		}
		else {
			_length = 0;
			_hash = 0;
			_dataIfHolder = (str && len) ? singletonEmptyStringHolder : allocateHolder(str, len);
		}
	}

	UT_StringRef(const UT_StringRef &src)
	{
		// copy membres
		if (holder()) {
			holder()->reference();
		}
	}

	virtual ~UT_StringRef(const UT_StringRef &src)
	{
		if (holder()) {
			holder()->unreference();
		}
	}

	const char *c_str() const
	{
		return holder() ? holder()->c_str() : _dataIfChars;
	}

	const char *length() const
	{
		return holder() ? holder()->length() : _length;
	}

	const char *hash() const
	{
		return holder() ? holder()->hash() : _hash;
	}

	const Holder *holder() const
	{
		return _length ? nullptr : _dataIfHolder;
	}
};
