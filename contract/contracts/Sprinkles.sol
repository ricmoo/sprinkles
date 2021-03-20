// contracts/GameItem.sol
// SPDX-License-Identifier: MIT
pragma solidity ^0.7.3;

import "@openzeppelin/contracts/token/ERC721/ERC721.sol";
import "@openzeppelin/contracts/utils/Counters.sol";

contract Sprinkles is ERC721 {

    event RegisterSprinkle(bytes32 indexed publicKeyHash, uint256 tokenId);

    mapping (uint256 => bytes32) _publicKeyHashes;

    address public owner;

    uint256 _nextTokenId;

    constructor() ERC721("Sprinkles", "SPKL") {
        owner = msg.sender;
        _nextTokenId = 1;
        _setBaseURI("https://metadata.sprinkles.network/");
    }

    function mint(address to, bytes32 publicKeyHash) public {
        require(msg.sender == owner);

        uint256 tokenId = _nextTokenId++;

        _publicKeyHashes[tokenId] = publicKeyHash;

        _mint(to, tokenId);

        RegisterSprinkle(publicKeyHash, tokenId);
    }

    function getPublicKeyHash(uint256 tokenId) public view returns (bytes32) {
        return _publicKeyHashes[tokenId];
    }
}

