#include "Mode.hpp"
#include "Dispatcher.hpp"
#include <stdexcept>
#include <iostream>

Mode::Mode() : score(0) {

}

Mode Mode::benchmark() {
	Mode r;
	r.name = "benchmark";
	r.kernel = "profanity_score_benchmark";
	return r;
}

Mode Mode::zeros() {
	Mode r = range(0, 0);
	r.name = "zeros";
	return r;
}

static std::string::size_type hexValueNoException(char c) {
	if (c >= 'A' && c <= 'F') {
		c -= 'A' - 'a';
	}

	const std::string hex = "0123456789abcdef";
	const std::string::size_type ret = hex.find(c);
	return ret;
}

static std::string::size_type hexValue(char c) {
	const std::string::size_type ret = hexValueNoException(c);
	if(ret == std::string::npos) {
		throw std::runtime_error("bad hex value");
	}

	return ret;
}

Mode Mode::matching(const std::string strHex) {
	Mode r;
	r.name = "matching";
	r.kernel = "profanity_score_matching";

	std::fill( r.data1, r.data1 + sizeof(r.data1), cl_uchar(0) );
	std::fill( r.data2, r.data2 + sizeof(r.data2), cl_uchar(0) );

	auto index = 0;
	
	for( size_t i = 0; i < strHex.size(); i += 2 ) {
		const auto indexHi = hexValueNoException(strHex[i]);
		const auto indexLo = i + 1 < strHex.size() ? hexValueNoException(strHex[i+1]) : std::string::npos;

		const auto valHi = (indexHi == std::string::npos) ? 0 : indexHi << 4;
		const auto valLo = (indexLo == std::string::npos) ? 0 : indexLo;

		const auto maskHi = (indexHi == std::string::npos) ? 0 : 0xF << 4;
		const auto maskLo = (indexLo == std::string::npos) ? 0 : 0xF;

		r.data1[index] = maskHi | maskLo;
		r.data2[index] = valHi | valLo;

		++index;
	}

	return r;
}

unsigned char sameBitsMask(unsigned char a, unsigned char b) {
	unsigned char r = 0;
	size_t mask = 1;
	
	for(size_t bit = 0; (bit < 8); ++bit) {
		r |= mask;
		if((a & mask) != (b & mask)) break;
		mask = mask << 1;
	}

	return r;
}

unsigned int sameBitsMask(unsigned int a, unsigned int b) {
	unsigned int r = 0;
	unsigned int mask = 1;
	
	for(size_t bit = 0; (bit < 32); ++bit) {
		r |= mask;
		if((a & mask) != (b & mask)) break;
		mask = mask << 1;
	}

	return r;
}


Mode Mode::tron_prefix(const std::string prefix) {
	if(prefix.size() < 1 || prefix.size() > 34) {
		throw std::runtime_error("tron_prefix wrong format: " + prefix);
	}

	Mode r;
	r.name = "tron_prefix";
	r.kernel = "profanity_score_tron_prefix";

	std::fill( r.data1, r.data1 + sizeof(r.data1), cl_uchar(0) );
	std::fill( r.data2, r.data2 + sizeof(r.data2), cl_uchar(0) );
	
	if(prefix[0] == 'T') {
		char* psz = new char[35];
		psz[34] = '\0';
		memcpy(&psz[0], prefix.c_str(), prefix.size());
		size_t TRON_ADDR_SIZE = 25;
		
		std::vector<unsigned char> lowest(TRON_ADDR_SIZE);
		//std::fill( &psz[prefix.size()], &psz + 34, '1' );
		memset(&psz[prefix.size()], '1', 34 - prefix.size());
		if(!Dispatcher::DecodeBase58(psz, lowest, TRON_ADDR_SIZE)) {
			std::cout << "Fail to Decode Base58: " << std::string(psz, 34) << std::endl;
			return Mode::benchmark();
		}
		
		std::vector<unsigned char> highest(TRON_ADDR_SIZE);
		//std::fill( psz + prefix.size(), psz + 34, cl_uchar('z') );
		//memset(&psz[prefix.size()], 'z', 34 - prefix.size());
		psz[prefix.size() - 1]++;
		if(!Dispatcher::DecodeBase58(psz, highest, TRON_ADDR_SIZE)) {
			std::cout << "Fail to Decode Base58: " << std::string(psz, 34) << std::endl;
			return Mode::benchmark();
		}

		std::vector<unsigned char> samebits(TRON_ADDR_SIZE - 1);
		memset(&samebits[0], '\0', TRON_ADDR_SIZE - 1);
		unsigned int* a = (unsigned int*) &lowest[1];
		unsigned int* b = (unsigned int*) &highest[1];
		unsigned int* dst = (unsigned int*) &samebits[0];
		for(size_t i = 0; i < 6; ++i) {
			dst[i] = sameBitsMask(a[i], b[i]);
			if(dst[i] != 0xFFFFFFFF) break;
		}
		//dst[1] = sameBitsMask(a[1], b[1]);
		/*for(size_t i = 0; i < TRON_ADDR_SIZE; ++i) {
			samebits[i] = sameBitsMask(lowest[i], highest[i]);
			if(samebits[i] != 0xFF) break;
		}*/

#ifdef PROFANITY_DEBUG
		std::cout << "tron_prefix lo: 0x" << Dispatcher::toHex(&lowest[0], TRON_ADDR_SIZE) << std::endl;
		std::cout << "tron_prefix hi: 0x" << Dispatcher::toHex(&highest[0], TRON_ADDR_SIZE) << std::endl;
		std::cout << "tron_prefix ~~:     " << Dispatcher::toHex(&samebits[0], TRON_ADDR_SIZE - 1) << std::endl;
#endif
		//std::cout << "tron_prefix range " << a[0] << "-" << b[0] << std::endl;
		for(size_t i = 0; (i < 20) && (samebits[i] != 0); ++i) {
			if(samebits[i] == 0xff) {
				r.data1[i] = samebits[i];
				r.data2[i] = lowest[i + 1];
#ifdef PROFANITY_DEBUG
				//std::cout << " data#" << i << " : x & " << Dispatcher::toHex(&r.data1[i], 1) << " = " << Dispatcher::toHex(&r.data2[i], 1) << std::endl;
				std::cout << " data#" << i << " : x = " << Dispatcher::toHex(&r.data2[i], 1) << std::endl;
#endif
			} else {
				r.data1[i] = highest[i + 1];
				r.data2[i] = lowest[i + 1];
#ifdef PROFANITY_DEBUG
				std::cout << " data#" << i << " : x >= " << Dispatcher::toHex(&r.data2[i], 1) << " && x < " << Dispatcher::toHex(&r.data1[i], 1) << std::endl;
#endif
			}
		}
		
		return r;
	}

	throw std::runtime_error("tron_prefix wrong format: " + prefix);
}

Mode Mode::leading(const char charLeading) {
	Mode r;
	r.name = "leading";
	r.kernel = "profanity_score_leading";
	r.data1[0] = static_cast<cl_uchar>(hexValue(charLeading));
	return r;
}

Mode Mode::gas() {
	Mode r;
	r.name = "gas";
	r.kernel = "profanity_score_gas";
	return r;
}

Mode Mode::range(const cl_uchar min, const cl_uchar max) {
	Mode r;
	r.name = "range";
	r.kernel = "profanity_score_range";
	r.data1[0] = min;
	r.data2[0] = max;
	return r;
}

Mode Mode::letters() {
	Mode r = range(10, 15);
	r.name = "letters";
	return r;
}

Mode Mode::numbers() {
	Mode r = range(0, 9);
	r.name = "numbers";
	return r;
}

std::string Mode::transformKernel() const {
	switch (this->target) {
		case ADDRESS:
			return "";
		case CONTRACT:
			return "profanity_transform_contract";
		default:
			throw "No kernel for target";
	}
}

std::string Mode::transformName() const {
	switch (this->target) {
		case ADDRESS:
			return "Address";
		case CONTRACT:
			return "Contract";
		default:
			throw "No name for target";
	}
}

Mode Mode::leadingRange(const cl_uchar min, const cl_uchar max) {
	Mode r;
	r.name = "leadingrange";
	r.kernel = "profanity_score_leadingrange";
	r.data1[0] = min;
	r.data2[0] = max;
	return r;
}

Mode Mode::mirror() {
	Mode r;
	r.name = "mirror";
	r.kernel = "profanity_score_mirror";
	return r;
}

Mode Mode::doubles() {
	Mode r;
	r.name = "doubles";
	r.kernel = "profanity_score_doubles";
	return r;
}
