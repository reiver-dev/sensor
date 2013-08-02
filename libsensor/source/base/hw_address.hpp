/*
 * hw_address.hpp
 *
 *  Created on: 11.12.2012
 *      Author: reiver
 */

#ifndef HW_ADDRESS_HPP_
#define HW_ADDRESS_HPP_

#include <net/ethernet.h>
#include <cstring>
#include <cstdint>

class HwAddress {
public:

	HwAddress() {
		memset(&address, 0, sizeof(address));
	}

	HwAddress(struct ether_addr &hwaddr) : address(hwaddr) {/* */}

	HwAddress(const HwAddress &other) : address(other.address) {/* */}

	HwAddress& operator=(const HwAddress &other) {
		memcpy(&address, &other.address, ETH_ALEN);
		return *this;
	}

	void setAddr(struct ether_addr &hwaddr) {
		memcpy(&address, &hwaddr, ETH_ALEN);
	}

	const std::uint8_t* data() const {
		return address.ether_addr_octet;
	}

	std::uint8_t* raw() {
		return address.ether_addr_octet;
	}

	bool operator==(const HwAddress &other) {
		return memcmp(&address, &other.address, sizeof(address)) == 0;
	}

private:

	struct ether_addr address;

};

#endif /* HW_ADDRESS_HPP_ */
