#ifndef _READER_H_
#define _READER_H_

template <typename It>
class Reader {

public:
    std::string next();
    std::string peek() const;

};

#endif // _READER_H_
