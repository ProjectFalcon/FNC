// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Copyright (c) 2020 The Falcon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <chainparamsseeds.h>
#include <consensus/merkle.h>
// #include <progressbar.hpp>
#include <tinyformat.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <util/moneystr.h>
#include <versionbitsinfo.h>


#include <assert.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

const bool mineGenesis = false;
int CChainParams::GetCoinYearPercent(int year) const
{
    if(static_cast<std::size_t>(year) < nBlockPerc.size()) {
        return nBlockPerc[year];
    } else {
        return 10;
    }
};

CAmount CChainParams::GetBaseBlockReward() const
{
    return nBlockReward;
};

CAmount CChainParams::GetProofOfStakeRewardAtYear(const int year) const
{
    auto x = (GetBaseBlockReward() * GetCoinYearPercent(year)) / 100;
    return x;
};

CAmount CChainParams::GetProofOfStakeRewardAtHeight(const int nHeight) const
{
    const CAmount nBlocksInAYear = (365 * 24 * 60 * 60) / GetTargetSpacing();
    const int currYear = nHeight / nBlocksInAYear;
    CAmount nSubsidy = GetProofOfStakeRewardAtYear(currYear);

    return nSubsidy;
}

int64_t CChainParams::GetProofOfStakeReward(const CBlockIndex *pindexPrev, const int64_t nFees) const
{
    int nHeight = pindexPrev ? pindexPrev->nHeight + 1 : 0;
    return GetProofOfStakeRewardAtHeight(nHeight) + nFees;
};

int64_t CChainParams::GetMaxSmsgFeeRateDelta(int64_t smsg_fee_prev) const
{
    return (smsg_fee_prev * consensus.smsg_fee_max_delta_percent) / 1000000;
};

const DevFundSettings *CChainParams::GetDevFundSettings(int64_t nTime,int nHeight) const
{
    for (auto i = vDevFundSettings.rbegin(); i != vDevFundSettings.rend(); ++i) {
        if (nTime > i->first) {
            return &i->second;
        }
    }

    return nullptr;
};

bool CChainParams::IsBech32Prefix(const std::vector<unsigned char> &vchPrefixIn) const
{
    for (auto &hrp : bech32Prefixes)  {
        if (vchPrefixIn == hrp) {
            return true;
        }
    }

    return false;
};

bool CChainParams::IsBech32Prefix(const std::vector<unsigned char> &vchPrefixIn, CChainParams::Base58Type &rtype) const
{
    for (size_t k = 0; k < MAX_BASE58_TYPES; ++k) {
        auto &hrp = bech32Prefixes[k];
        if (vchPrefixIn == hrp) {
            rtype = static_cast<CChainParams::Base58Type>(k);
            return true;
        }
    }

    return false;
};

bool CChainParams::IsBech32Prefix(const char *ps, size_t slen, CChainParams::Base58Type &rtype) const
{
    for (size_t k = 0; k < MAX_BASE58_TYPES; ++k) {
        const auto &hrp = bech32Prefixes[k];
        size_t hrplen = hrp.size();
        if (hrplen > 0
            && slen > hrplen
            && strncmp(ps, (const char*)&hrp[0], hrplen) == 0) {
            rtype = static_cast<CChainParams::Base58Type>(k);
            return true;
        }
    }

    return false;
};

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

const std::pair<const char*, CAmount> regTestOutputs[] = {
    std::make_pair("585c2b3914d9ee51f8e710304e386531c3abcc82", 10000 * COIN),
    std::make_pair("c33f3603ce7c46b423536f0434155dad8ee2aa1f", 10000 * COIN),
    std::make_pair("72d83540ed1dcf28bfaca3fa2ed77100c2808825", 10000 * COIN),
    std::make_pair("69e4cc4c219d8971a253cd5db69a0c99c4a5659d", 10000 * COIN),
    std::make_pair("eab5ed88d97e50c87615a015771e220ab0a0991a", 10000 * COIN),
    std::make_pair("119668a93761a34a4ba1c065794b26733975904f", 10000 * COIN),
    std::make_pair("6da49762a4402d199d41d5778fcb69de19abbe9f", 10000 * COIN),
    std::make_pair("27974d10ff5ba65052be7461d89ef2185acbe411", 10000 * COIN),
    std::make_pair("89ea3129b8dbf1238b20a50211d50d462a988f61", 10000 * COIN),
    std::make_pair("3baab5b42a409b7c6848a95dfd06ff792511d561", 10000 * COIN),

    std::make_pair("649b801848cc0c32993fb39927654969a5af27b0", 5000 * COIN),
    std::make_pair("d669de30fa30c3e64a0303cb13df12391a2f7256", 5000 * COIN),
    std::make_pair("f0c0e3ebe4a1334ed6a5e9c1e069ef425c529934", 5000 * COIN),
    std::make_pair("27189afe71ca423856de5f17538a069f22385422", 5000 * COIN),
    std::make_pair("0e7f6fe0c4a5a6a9bfd18f7effdd5898b1f40b80", 5000 * COIN),
};
const size_t nGenesisOutputsRegtest = sizeof(regTestOutputs) / sizeof(regTestOutputs[0]);

const std::pair<const char*, CAmount> genesisOutputs[] = {
    std::make_pair("46792e37c1bbe44294a9a020395f38c13cb791c4", 4500000000 * COIN),//FCFjw11L3ApZWcLM2ZLWQc9Qjv8SQp5Lgz
    std::make_pair("830e35ce278978b618577ab8c582aecee72134f7", 4500000000 * COIN),//FHn51Ghnrzp56xAEuWXsfL42SLBN19uaiS
    std::make_pair("8a4ea720ee6f73ad59617a120907c4bc970b0174", 4500000000 * COIN),//FJSQwAnaU4yKEbZV77Bxqd3daHCwcLwJC7
    std::make_pair("3e7b7237774ee02e43996c7d2212540c142b6e5c", 4500000000 * COIN),//FBXVEtFCSpiFaadoKLWG1BGWz9TUj4FKP8
    std::make_pair("2f2959e006ad885ec557a0c38986049677aaed34", 4500000000 * COIN),//FA8UkwVtmuAe1NB1ShYb7f6vV8c8GCDvQf
    std::make_pair("f447ffeaa316f5ea9475dc9b50e881a66c8e47cf", 4500000000 * COIN),//FU6kbEUEy2MhPT7VmXv24sbQUFkRct1d8j
    std::make_pair("5ec9c41efa472f7baec523dce8c933138ae871fb", 4500000000 * COIN),//FEUJgndysB1vzyg3VDhX9oQs1jKq3GAtLa
    std::make_pair("3b41c382d00004435475d170654b05ea243f5f6b", 4500000000 * COIN),//FBES7TmhjTsrAUP69RnK7cFyoJkykpRYda
    std::make_pair("16c52c929034ca169e8b223634d8bf188d0a2b6b", 4500000000 * COIN),//F7uWXvG39YNgv1Chr2zNumAZcae4efSKAL
    std::make_pair("0ef161911b81430d5262b2b40c303168fd470183", 4500000000 * COIN),//F7C85wYh9zrxF4R1dcRdmdcfpnnUBZuDfX
    std::make_pair("abe4453cd7d39d14c8f855d9610ffd0c246ed20f", 4500000000 * COIN),//FMVzU7Ze7tHwMfsWfUbqQQuBcaGh1WtV3n
    std::make_pair("be7f5ac23942e474a16373c82b0c9766ad811de9", 4500000000 * COIN),//FPCNQzmMVbTb4B3PMydhfSXQX5BiJpvxnz
    std::make_pair("332a2002e2bfc6c3133488a6190b370fe306b624", 4500000000 * COIN),//FAVePV2mG8oKDp58YxMKoMZPCbV3KRD4a1
    std::make_pair("6943b1df58bab603c12b583966cb55d30cf6e2f9", 4500000000 * COIN),//FFRhWTjVLEWS6XXKfzXJTyfDLBV6uGpbJh
    std::make_pair("cec904717020c95731f1487e5af1d7e479dc3a63", 4500000000 * COIN),//FQgVUFQXXvXuhNuaaV9oMcYPhN7oBk911b
    std::make_pair("6adc90736bc109ee8c52d99f7616572b8cdd22dd", 4500000000 * COIN),//FFa9KD57KoX9VstLRyjEXPMugjqYeBbrFE
    std::make_pair("4f31f359515dfaa1253b239e50e79a313c24b9f6", 4500000000 * COIN),//FD3rganFx3hqRnm7QddMnDSxQaAQGDyvGp
    std::make_pair("f9fc64b05dc8b544f66c5551c2e5c339ace17f1f", 4500000000 * COIN),//FUcv5AMsYdv2bLnKC4xR5Cixotp6U6JZ3v
    std::make_pair("9f95629b8cff68cb00b0fb1b5e051ad839d258a3", 4500000000 * COIN),//FLNurmSnTjvyKR1vMQve5PEvSywHjWsZMe
    std::make_pair("f507e5220d807fb643d5d6fbe99744ca9d8c471d", 4500000000 * COIN),//FUAiUMh9j9K9gjFfUYEnmewKR2a7SXWJc9

};
const size_t nGenesisOutputs = sizeof(genesisOutputs) / sizeof(genesisOutputs[0]);

const std::pair<const char*, CAmount> genesisOutputsTestnet[] = {
    std::make_pair("8ae2036c06028e20ac32e01bd59ed9e09291c6c5", 678670.75 * COIN),//XQ1arJSZbaWqKJFJkuQtuHMbcith7ZMPkS
    std::make_pair("c7c52417d63147ddd43f9449b19c0a286dad8740", 678670.75 * COIN),//XVZXUv2r3Bwc3TuePGPm19ZoYctXrtYBD4
    std::make_pair("0affbdd2a3f88716989388397bdce0923e482a23", 678670.75 * COIN),//XCMPyHzZwGoe7YY7xDq6DnQ9uiX5G4fqW9
    std::make_pair("1ecc333c09a3eaccd0dcd1d57ff6109502b3aa62", 678670.75 * COIN),//XEA5jySRAx4DMcJBttQ19UEbxzpJg7WsZE
    std::make_pair("1c83b0ea73814a4c06f39680151afb5414f9c2db", 678670.75 * COIN),//XDx1XaAUGLXDTBbXTvc712Jm3S54SrT5qk
    std::make_pair("501c9c2b742c673b287fd001daab061a9a990287", 678670.75 * COIN),//XJeqBMM3Acqa9VCdbWeNyxUY2Usth73irW
    std::make_pair("dc618eca61baa03fb2fc780fb02af59d428d52a1", 678670.75 * COIN),//XXSWNnEMaHT6JaAkQbMYUamKFuzXsCWo4o
    std::make_pair("f0bb2b0e5b978a50103c19d6726ad33fbcf27172", 678670.75 * COIN),//XZJ7Eri4vRhHmFrsdNnxf73U7WtDJa4fx6
    std::make_pair("91059d85a87905e3aeb5c07526b32d2443ccb169", 678670.75 * COIN),//XQa3Z7tcWSmSWMEdpicKG5Z51KvdPvBJQF
    std::make_pair("ef901834835e8f51519da5741bd5511dfdff22c6", 678670.75 * COIN),//XZBvxrgAgDUJzeh9Ba7L9ezkNLCxvDEAii
    std::make_pair("3b5d7439fcca520ad989d49038bc9dc483971060", 678670.75 * COIN),//XGm8fm7HZ58znQTg6XKWs2SSeVQmkKvUj3
    std::make_pair("1cb113bd0778fa09bbd4927411b7dba0a673f5c9", 678670.75 * COIN),//XDxwu3nuMWddnVSSnfdqj2KisJVtBKe3v4
    std::make_pair("12e5e9ce93d416d5ec4127f0e1d682972c8e1ddc", 678670.75 * COIN),//XD5ASFiXkRoLD6ecLtXKfmBYHbKtVwRcEz
    std::make_pair("8848ae9c76af612cd4e41f8e073f47e432c774cb", 678670.75 * COIN),//XPmqpGnKtGHoJHM6zxW8QiXZFBULkcs2hp
    std::make_pair("5619392124dc6a00243053742f3872fa0da270d9", 678670.75 * COIN),//XKCVBBJ5pHVfgh5nLsB3FRsPt2oqiq4jwK
    std::make_pair("babfa38ae73de9bb26fa21cf30dc535019e1833e", 678670.75 * COIN),//XUNg78DaeQwXVJ3LmgEaxRsPwFitxbSSXa
    std::make_pair("14ea24f913c3af99e0d8744dd1405609426277a0", 678670.75 * COIN),//XDFprczaohtkFKkF5uzHcuijWYaMcPuzHc
    std::make_pair("79f906021c2cbbe1c5405ca8a7f44548ee79a82f", 678670.75 * COIN),//XNUAvoxJta9eTByKUa1orX5DUQ7uB77FXo
    std::make_pair("aca61ffbf935ec4bc102f6a3765a8e80408c1ecb", 678670.75 * COIN),//XT685hiSm3RqkcKvKnvxWEBgs9vq11ot1R
    std::make_pair("c19ec5255e21e93347d2704d8430f4f8bdae7dc3", 678670.75 * COIN),//XV11Ti2bhUCSGN5FHG2RvcP9eMXRVuqmiL
};
const size_t nGenesisOutputsTestnet = sizeof(genesisOutputsTestnet) / sizeof(genesisOutputsTestnet[0]);


static CBlock CreateGenesisBlockRegTest(uint32_t nTime, uint32_t nNonce, uint32_t nBits)
{
    const char *pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";

    CMutableTransaction txNew;
    txNew.nVersion = FALCON_TXN_VERSION;
    txNew.SetType(TXN_COINBASE);
    txNew.vin.resize(1);
    uint32_t nHeight = 0;  // bip34
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp)) << OP_RETURN << nHeight;

    txNew.vpout.resize(nGenesisOutputsRegtest);
    for (size_t k = 0; k < nGenesisOutputsRegtest; ++k) {
        OUTPUT_PTR<CTxOutStandard> out = MAKE_OUTPUT<CTxOutStandard>();
        out->nValue = regTestOutputs[k].second;
        out->scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ParseHex(regTestOutputs[k].first) << OP_EQUALVERIFY << OP_CHECKSIG;
        txNew.vpout[k] = out;
    }

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = FALCON_BLOCK_VERSION;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));

    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    genesis.hashWitnessMerkleRoot = BlockWitnessMerkleRoot(genesis);

    return genesis;
}

static CBlock CreateGenesisBlockTestNet(uint32_t nTime, uint32_t nNonce, uint32_t nBits)
{
    const char *pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";

    CMutableTransaction txNew;
    txNew.nVersion = FALCON_TXN_VERSION;
    txNew.SetType(TXN_COINBASE);
    txNew.vin.resize(1);
    uint32_t nHeight = 0;  // bip34
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp)) << OP_RETURN << nHeight;

    txNew.vpout.resize(nGenesisOutputsTestnet);
    for (size_t k = 0; k < nGenesisOutputsTestnet; ++k) {
        OUTPUT_PTR<CTxOutStandard> out = MAKE_OUTPUT<CTxOutStandard>();
        out->nValue = genesisOutputsTestnet[k].second;
        out->scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ParseHex(genesisOutputsTestnet[k].first) << OP_EQUALVERIFY << OP_CHECKSIG;
        txNew.vpout[k] = out;
    }

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = FALCON_BLOCK_VERSION;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));

    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    genesis.hashWitnessMerkleRoot = BlockWitnessMerkleRoot(genesis);

    return genesis;
}

static CBlock CreateGenesisBlockMainNet(uint32_t nTime, uint32_t nNonce, uint32_t nBits)
{
    const char *pszTimestamp = "BTC 000000000000000000c679bc2209676d05129834627c7b1c02d1018b224c6f37";

    CMutableTransaction txNew;
    txNew.nVersion = FALCON_TXN_VERSION;
    txNew.SetType(TXN_COINBASE);

    txNew.vin.resize(1);
    uint32_t nHeight = 0;  // bip34
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp)) << OP_RETURN << nHeight;

    txNew.vpout.resize(nGenesisOutputs);
    for (size_t k = 0; k < nGenesisOutputs; ++k) {
        OUTPUT_PTR<CTxOutStandard> out = MAKE_OUTPUT<CTxOutStandard>();
        out->nValue = genesisOutputs[k].second;
        out->scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ParseHex(genesisOutputs[k].first) << OP_EQUALVERIFY << OP_CHECKSIG;
        txNew.vpout[k] = out;
    }

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = FALCON_BLOCK_VERSION;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));

    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    genesis.hashWitnessMerkleRoot = BlockWitnessMerkleRoot(genesis);

    return genesis;
}

/* Helper functions to simplify new genesis block finding*/

static CBlock CreateGenesisBlockForNet(std::string net,uint32_t nTime, uint32_t nNonce, uint32_t nBits){
    if(net == "mainnet")
        return CreateGenesisBlockMainNet(nTime,nNonce,nBits);
    else if (net == "testnet")
        return CreateGenesisBlockTestNet(nTime,nNonce,nBits);
    else if (net == "regtest")
        return CreateGenesisBlockRegTest(nTime,nNonce,nBits);
    else
        throw std::runtime_error("Invalid network given");
}

std::string getCreteGenStringForNetwork(std::string net){
    if(net == "mainnet")
        return "CreateGenesisBlockMainNet";
    else if (net == "testnet")
        return "CreateGenesisBlockTestNet";
    else if (net == "regtest")
        return "CreateGenesisBlockRegTest";
    else
        throw std::runtime_error("Invalid network given");
}

std::string enclosestringinquotes(std::string str){
    return "\"" + str + "\"";
}

std::string generateGenesisCode(std::string net,CBlock genesisblock){
    std::string createfunc = getCreteGenStringForNetwork(net);
    std::string gensiscode = "";
    gensiscode += "        genesis = " + createfunc + "(" + std::to_string(genesisblock.nTime) + ", " + std::to_string(genesisblock.nNonce) + ", " + std::to_string(genesisblock.nBits) + ");\n";
    gensiscode += "        consensus.hashGenesisBlock = genesis.GetHash();\n";
    gensiscode += "        if(!mineGenesis) {\n";
    gensiscode += "           assert(consensus.hashGenesisBlock == uint256S(" + enclosestringinquotes(genesisblock.GetHash().ToString())     +"));\n" ;
    gensiscode += "           assert(genesis.hashMerkleRoot == uint256S(" + enclosestringinquotes(genesisblock.hashMerkleRoot.ToString())  +"));\n" ;
    gensiscode += "           assert(genesis.hashWitnessMerkleRoot == uint256S(" + enclosestringinquotes(genesisblock.hashWitnessMerkleRoot.ToString())  +"));\n" ;
    gensiscode += "        }";
    return gensiscode;
}

void FindGenesis(std::string network,Consensus::Params consensus,CBlock genesisblock,uint32_t nGenesisTime,uint32_t nBits,bool fExitOnSol){
        arith_uint256 test;
        bool fNegative;
        bool fOverflow;
        test.SetCompact(nBits, &fNegative, &fOverflow);
        // std::cout << "Test threshold: " << test.GetHex() << "\n\n";
        int genesisNonce = 0;
        uint256 TempHashHolding = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");
        uint256 BestBlockHash = uint256S("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        // std::cout << "Finding genesis block for network " << network << "\n";
        int nMaxNonce = 40000000;
        for (int i=0;i<nMaxNonce;i++) {
            genesisblock = CreateGenesisBlockForNet(network,nGenesisTime, i, nBits);
            consensus.hashGenesisBlock = genesisblock.GetHash();
            arith_uint256 BestBlockHashArith = UintToArith256(BestBlockHash);
            if (UintToArith256(consensus.hashGenesisBlock) < BestBlockHashArith) {
                BestBlockHash = consensus.hashGenesisBlock;
                // std::cout << "Nonce testing is " << i << " \n";
            }
            TempHashHolding = consensus.hashGenesisBlock;
            std::cout.clear();
            if (BestBlockHashArith < test) {
                genesisNonce = i - 1;
                genesisblock = CreateGenesisBlockForNet(network,nGenesisTime, genesisNonce, nBits);
                break;
            }
        }
        std::cout << "\n";
        std::cout << "Genesis Block details for " << network << "\n";
        std::cout << generateGenesisCode(network,genesisblock);
        std::cout << "\n";
        if(fExitOnSol)
            std::exit(0);
}

/* End helpers */
/**
 * Main network
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP34Height = 0;
        consensus.BIP65Height = 0;
        consensus.BIP66Height = 0;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 0;

        consensus.OpIsCoinstakeTime = 0x5A04EC00;       // 2017-11-10 00:00:00 UTC
        consensus.fAllowOpIsCoinstakeWithP2PKH = false;
        consensus.nPaidSmsgTime = 0x5C791EC0;           // 2019-03-01 12:00:00
        consensus.csp2shTime = 0x5C791EC0;              // 2019-03-01 12:00:00
        consensus.smsg_fee_time = 0x5D2DBC40;           // 2019-07-16 12:00:00
        consensus.bulletproof_time = 0x5D2DBC40;        // 2019-07-16 12:00:00
        consensus.rct_time = 0x5D2DBC40;                // 2019-07-16 12:00:00
        consensus.smsg_difficulty_time = 0x5D2DBC40;    // 2019-07-16 12:00:00

        consensus.smsg_fee_period = 5040;
        consensus.smsg_fee_funding_tx_per_k = 200000;
        consensus.smsg_fee_msg_per_day_per_k = 50000;
        consensus.smsg_fee_max_delta_percent = 43;
        consensus.smsg_min_difficulty = 0x1effffff;
        consensus.smsg_difficulty_max_delta = 0xffff;

        consensus.powLimit = uint256S("00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x0");//TODO akshaynexus set this on mainnet

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x0"); //TODO akshaynexus set this on mainnet

        consensus.nMinRCTOutputDepth = 12;

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xf2;
        pchMessageStart[1] = 0xf3;
        pchMessageStart[2] = 0xe1;
        pchMessageStart[3] = 0xb4;
        nDefaultPort = 51728;
        nBIP44ID = 0x8000002C;

        nModifierInterval = 10 * 60;    // 10 minutes
        nStakeMinConfirmations = 225;   // 225 * 2 minutes
        nTargetSpacing = 120;           // 2 minutes
        nTargetTimespan = 24 * 60;      // 24 mins
        consensus.nLWMADiffUpgradeHeight = 40863;
        consensus.nZawyLwmaAveragingWindow = 45;
        nBlockReward = 2000 * COIN;
        nBlockPerc = {100, 100, 95, 90, 86, 81, 77, 74, 70, 66, 63, 60, 57, 54, 51, 49, 46, 44, 42, 40, 38, 36, 34, 32, 31, 29, 28, 26, 25, 24, 23, 21, 20, 19, 18, 17, 17, 16, 15, 14, 14, 13, 12, 12, 11, 10, 10};

        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size = 1;
        m_assumed_chain_state_size = 1;
        if(mineGenesis)
            FindGenesis("mainnet",consensus,genesis,GetTime(),520159231,false);

        genesis = CreateGenesisBlockMainNet(1607412307, 155664, 520159231);
        consensus.hashGenesisBlock = genesis.GetHash();
        if(!mineGenesis) {
           assert(consensus.hashGenesisBlock == uint256S("0000c1a9b61bacee1aef8fe32aae04302ede31eccb800cb7b993367971c413cc"));
           assert(genesis.hashMerkleRoot == uint256S("e8a80a3db46511b92bf490d19d4098b0c33c462b56aa413e516371dd7b0d6ad3"));
           assert(genesis.hashWitnessMerkleRoot == uint256S("a7b1d06697763b2cb44df8419a4b233ecd86ea62785ab707605522883be22f68"));
        }
        // Note that of those which support the service bits prefix, most only support a subset of
        // possible options.
        // This is fine at runtime as we'll fall back to using them as a oneshot if they don't support the
        // service bits we want, but we should get them updated to support all service bits wanted by any
        // release ASAP to avoid it where possible.
        vSeeds.emplace_back("45.81.224.174");
        vSeeds.emplace_back("45.81.224.217");
/*
        //TODO akshaynexus Set this to falcon devfund
        vDevFundSettings.emplace_back(0,
            DevFundSettings("4b6a64d7724c3bf0472cb2ad31a4919518db3db5", 15, 360));//Approx each 12 hr payment to dev fund
*/
        base58Prefixes[PUBKEY_ADDRESS]     = {0x23}; // F
        base58Prefixes[SCRIPT_ADDRESS]     = {0x61}; // g
        base58Prefixes[PUBKEY_ADDRESS_256] = {0x39};
        base58Prefixes[SCRIPT_ADDRESS_256] = {0x3d};
        base58Prefixes[SECRET_KEY]         = {0xA6}; //PUBKEY_ADDRESS Prefix in int + 128 converted to hexadecimal
        base58Prefixes[EXT_PUBLIC_KEY]     = {0x68, 0xDF, 0x7C, 0xBD}; // PGHST
        base58Prefixes[EXT_SECRET_KEY]     = {0x8E, 0x8E, 0xA8, 0xEA}; // XGHST
        base58Prefixes[STEALTH_ADDRESS]    = {0x14};
        base58Prefixes[EXT_KEY_HASH]       = {0x4b}; // X
        base58Prefixes[EXT_ACC_HASH]       = {0x17}; // A
        base58Prefixes[EXT_PUBLIC_KEY_BTC] = {0x04, 0x88, 0xB2, 0x1E}; // xpub
        base58Prefixes[EXT_SECRET_KEY_BTC] = {0x04, 0x88, 0xAD, 0xE4}; // xprv

        {
            std::map<int, std::string> bech32PrefixesMap{
                {PUBKEY_ADDRESS, "gp"},
                {SCRIPT_ADDRESS,"gw"},
                {PUBKEY_ADDRESS_256,"gl"},
                {SCRIPT_ADDRESS_256,"gj"},
                {SECRET_KEY,"gtx"},
                {EXT_PUBLIC_KEY,"gep"},
                {EXT_SECRET_KEY,"gex"},
                {STEALTH_ADDRESS,"gx"},
                {EXT_KEY_HASH,"gek"},
                {EXT_ACC_HASH,"gea"},
                {STAKE_ONLY_PKADDR,"gcs"},
            };

            for(auto&& p: bech32PrefixesMap)
            {
                bech32Prefixes[p.first].assign(p.second.begin(), p.second.end());
            }
        }

        bech32_hrp = "gw";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = false;

        checkpointData = {
            {
                { 0, genesis.GetHash()},
            }
        };

        chainTxData = ChainTxData {
            // Data from rpc: getchaintxstats 2912 eccad59c62c2b669a746297d1f3ffb49c4de8620d6ad69c240079386130b2343
            /* nTime    */ 1593280240,
            /* nTxCount */ 3317,
            /* dTxRate  */ 0.008253214698037342
        };
    }

    void SetOld()
    {
        consensus.BIP16Exception = uint256S("0x00000000000002dc756eebf4f49723ed8d30cc28a5f108eb94b1ba88ac4f9c22");
        consensus.BIP34Height = 227931;
        consensus.BIP34Hash = uint256S("0x000000000000024b89b42a942fe0d9fea3bb44ab7bd1b19115dd6a759c0808b8");
        consensus.BIP65Height = 388381; // 000000000000000004c2b624ed5d7756c508d90fd0da2c7c679febfa6c4735f0
        consensus.BIP66Height = 363725; // 00000000000000000379eaa19dce8c9b722d46ae6a57c2f1a988119488b50931
        consensus.CSVHeight = 419328; // 000000000000000004a1b34462cb8aeebd5799177f7a29cf28f2d1961716b5b5
        consensus.SegwitHeight = 481824; // 0000000000000000001c8018d9cb3b742ef25114f27563e3fc4a1902167f9893
        consensus.MinBIP9WarningHeight = consensus.SegwitHeight + consensus.nMinerConfirmationWindow;
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

        genesis = CreateGenesisBlock(1231006505, 2083236893, 0x1d00ffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,0);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "bc";
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP34Height = 0;
        consensus.BIP65Height = 0;
        consensus.BIP66Height = 0;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 0;
        consensus.MinBIP9WarningHeight = 0;

        consensus.OpIsCoinstakeTime = 0;
        consensus.fAllowOpIsCoinstakeWithP2PKH = false;
        consensus.nPaidSmsgTime = 0;
        consensus.csp2shTime = 0x5C67FB40;              // 2019-02-16 12:00:00
        consensus.smsg_fee_time = 0x5C67FB40;           // 2019-02-16 12:00:00
        consensus.bulletproof_time = 0x5C67FB40;        // 2019-02-16 12:00:00
        consensus.rct_time = 0;
        consensus.smsg_difficulty_time = 0x5D19F5C0;    // 2019-07-01 12:00:00

        consensus.smsg_fee_period = 5040;
        consensus.smsg_fee_funding_tx_per_k = 200000;
        consensus.smsg_fee_msg_per_day_per_k = 50000;
        consensus.smsg_fee_max_delta_percent = 43;
        consensus.smsg_min_difficulty = 0x1effffff;
        consensus.smsg_difficulty_max_delta = 0xffff;

        consensus.powLimit = uint256S("00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // The best chain should have at least this much work.
        //TODO akshaynexus fill this value once testnet is up
        consensus.nMinimumChainWork = uint256S("0x0");

        // By default assume that the signatures in ancestors of this block are valid.
        //TODO akshaynexus fill this value once testnet is up
        consensus.defaultAssumeValid = uint256S("0x0");

        consensus.nMinRCTOutputDepth = 12;
        //TODO akshaynexus change pchmessage start,bip44id and p2p port
        pchMessageStart[0] = 0xfb;
        pchMessageStart[1] = 0xc1;
        pchMessageStart[2] = 0xb4;
        pchMessageStart[3] = 0xdd;
        nDefaultPort = 51928;
        nBIP44ID = 0x80000213;

        nModifierInterval = 10 * 60;    // 10 minutes
        nStakeMinConfirmations = 225;   // 225 * 2 minutes
        nTargetSpacing = 120;           // 2 minutes
        nTargetTimespan = 24 * 60;      // 24 mins
        consensus.nZawyLwmaAveragingWindow = 45;
        consensus.nLWMADiffUpgradeHeight = 49512;//TODO akshaynexus set this around 100 blocks after genesis
        nBlockReward = 2000 * COIN;
        nBlockPerc = {100, 100, 95, 90, 86, 81, 77, 74, 70, 66, 63, 60, 57, 54, 51, 49, 46, 44, 42, 40, 38, 36, 34, 32, 31, 29, 28, 26, 25, 24, 23, 21, 20, 19, 18, 17, 17, 16, 15, 14, 14, 13, 12, 12, 11, 10, 10};

        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 1;
        m_assumed_chain_state_size = 1;
        /*
        //TODO akshaynexus generate new genesis block for FNC testnet
*/
        if(mineGenesis)
            FindGenesis("testnet",consensus,genesis,GetTime(),0x1f00ffff,false);

        genesis = CreateGenesisBlockTestNet(1607412309, 34175, 520159231);
        consensus.hashGenesisBlock = genesis.GetHash();
        if(!mineGenesis) {
           assert(consensus.hashGenesisBlock == uint256S("0000e305cf1a6e74e0211280bcf6b2ba5ee4c8596a117a45ca45c4c61e845c63"));
           assert(genesis.hashMerkleRoot == uint256S("a4ac827e3f40b55d69013e74f29fac184dbcd4b5e2f726c4273ffbc8c0779475"));
           assert(genesis.hashWitnessMerkleRoot == uint256S("08917a20ac55e83d7d860b7daf0cb3c25114d00338deab02f77af9eea20c06e1"));
        }

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.emplace_back("falcon-testnetdns.mineit.io");

        vDevFundSettings.push_back(std::make_pair(0, DevFundSettings("XHjYLwbVGbhr96HZqhT7j8crjEZJiGdZ1B", 15.00, 1440)));

        base58Prefixes[PUBKEY_ADDRESS]     = {0x4B}; // X
        base58Prefixes[SCRIPT_ADDRESS]     = {0x89}; // x
        base58Prefixes[PUBKEY_ADDRESS_256] = {0x77};
        base58Prefixes[SCRIPT_ADDRESS_256] = {0x7b};
        base58Prefixes[SECRET_KEY]         = {0x2e};
        base58Prefixes[EXT_PUBLIC_KEY]     = {0xe1, 0x42, 0x78, 0x00}; // ppar
        base58Prefixes[EXT_SECRET_KEY]     = {0x04, 0x88, 0x94, 0x78}; // xpar
        base58Prefixes[STEALTH_ADDRESS]    = {0x15}; // T
        base58Prefixes[EXT_KEY_HASH]       = {0x89}; // x
        base58Prefixes[EXT_ACC_HASH]       = {0x53}; // a
        base58Prefixes[EXT_PUBLIC_KEY_BTC] = {0x04, 0x35, 0x87, 0xCF}; // tpub
        base58Prefixes[EXT_SECRET_KEY_BTC] = {0x04, 0x35, 0x83, 0x94}; // tprv

        {
            std::map<int, std::string> bech32PrefixesMap{
                {PUBKEY_ADDRESS, "tfalcon"},
                {SCRIPT_ADDRESS,"tw"},
                {PUBKEY_ADDRESS_256,"tl"},
                {SCRIPT_ADDRESS_256,"tj"},
                {SECRET_KEY,"ttx"},
                {EXT_PUBLIC_KEY,"tep"},
                {EXT_SECRET_KEY,"tex"},
                {STEALTH_ADDRESS,"ts"},
                {EXT_KEY_HASH,"tek"},
                {EXT_ACC_HASH,"tea"},
                {STAKE_ONLY_PKADDR,"tcs"},
            };

            for(auto&& p: bech32PrefixesMap)
            {
                bech32Prefixes[p.first].assign(p.second.begin(), p.second.end());
            }
        }

        bech32_hrp = "tw";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        m_is_test_chain = true;

        checkpointData = {
            {
                {0, consensus.hashGenesisBlock},
            }
        };

        chainTxData = ChainTxData{
            // Data from rpc: getchaintxstats 4096 1d9a63069ed2b88c9a1752ade780d39f783118a4d6f7b4a04b398c3d77d4cd1f
            /* nTime    */ 1592655872,
            /* nTxCount */ 14905,
            /* dTxRate  */ 0.007782233656174334
        };
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    explicit CRegTestParams(const ArgsManager& args) {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;
        consensus.BIP16Exception = uint256();
        consensus.BIP34Height = 500; // BIP34 activated on regtest (Used in functional tests)
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in functional tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in functional tests)
        consensus.CSVHeight = 432; // CSV activated on regtest (Used in rpc activation tests)
        consensus.SegwitHeight = 0; // SEGWIT is always activated on regtest unless overridden
        consensus.MinBIP9WarningHeight = 0;

        consensus.OpIsCoinstakeTime = 0;
        consensus.fAllowOpIsCoinstakeWithP2PKH = false;
        consensus.nPaidSmsgTime = 0;
        consensus.csp2shTime = 0;
        consensus.smsg_fee_time = 0;
        consensus.bulletproof_time = 0;
        consensus.rct_time = 0;
        consensus.smsg_difficulty_time = 0;

        consensus.smsg_fee_period = 50;
        consensus.smsg_fee_funding_tx_per_k = 200000;
        consensus.smsg_fee_msg_per_day_per_k = 50000;
        consensus.smsg_fee_max_delta_percent = 4300;
        consensus.smsg_min_difficulty = 0x1f0fffff;
        consensus.smsg_difficulty_max_delta = 0xffff;

        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        consensus.nMinRCTOutputDepth = 2;

        pchMessageStart[0] = 0x09;
        pchMessageStart[1] = 0x12;
        pchMessageStart[2] = 0x06;
        pchMessageStart[3] = 0x0c;
        nDefaultPort = 11928;
        nBIP44ID = 0x80000001;


        nModifierInterval = 2 * 60;     // 2 minutes
        nStakeMinConfirmations = 12;
        nTargetSpacing = 5;             // 5 seconds
        nTargetTimespan = 16 * 60;      // 16 mins
        nStakeTimestampMask = 0;
        nBlockReward = 6 * COIN;
        consensus.nLWMADiffUpgradeHeight = INT_MAX;//TODO akshaynexus set regtest height
        consensus.nZawyLwmaAveragingWindow = 45;
        nBlockPerc = {100, 100, 95, 90, 86, 81, 77, 74, 70, 66, 63, 60, 57, 54, 51, 49, 46, 44, 42, 40, 38, 36, 34, 32, 31, 29, 28, 26, 25, 24, 23, 21, 20, 19, 18, 17, 17, 16, 15, 14, 14, 13, 12, 12, 11, 10, 10};
        //DevFund settings before gvr addition
        //Commented out regtest,uncomment to test gvr one time pay
            //     vDevFundSettings.emplace_back(0,
            // DevFundSettings("pZT7cC5oPiadxPkM6u2WDBrRN19oG1ZsNF", 33.00, 2));//Approx each 12 hr payment to dev fund

        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        UpdateActivationParametersFromArgs(args);
        if(mineGenesis)
            FindGenesis("regtest",consensus,genesis,GetTime(),545259519,true);

        genesis = CreateGenesisBlockRegTest(1607412309, 0, 545259519);
        consensus.hashGenesisBlock = genesis.GetHash();
        if(!mineGenesis) {
           assert(consensus.hashGenesisBlock == uint256S("6f68baecffa19b772e3e13015812709f705385dc33fde6a588fafed96ca0fb95"));
           assert(genesis.hashMerkleRoot == uint256S("f89653c7208af2c76a3070d436229fb782acbd065bd5810307995b9982423ce7"));
           assert(genesis.hashWitnessMerkleRoot == uint256S("36b66a1aff91f34ab794da710d007777ef5e612a320e1979ac96e5f292399639"));
        }

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = true;
        m_is_test_chain = true;

        checkpointData = {
            {
                {0, consensus.hashGenesisBlock},
            }
        };

        base58Prefixes[PUBKEY_ADDRESS]     = {0x76}; // p
        base58Prefixes[SCRIPT_ADDRESS]     = {0x7a};
        base58Prefixes[PUBKEY_ADDRESS_256] = {0x77};
        base58Prefixes[SCRIPT_ADDRESS_256] = {0x7b};
        base58Prefixes[SECRET_KEY]         = {0x2e};
        base58Prefixes[EXT_PUBLIC_KEY]     = {0xe1, 0x42, 0x78, 0x00}; // ppar
        base58Prefixes[EXT_SECRET_KEY]     = {0x04, 0x88, 0x94, 0x78}; // xpar
        base58Prefixes[STEALTH_ADDRESS]    = {0x15}; // T
        base58Prefixes[EXT_KEY_HASH]       = {0x89}; // x
        base58Prefixes[EXT_ACC_HASH]       = {0x53}; // a
        base58Prefixes[EXT_PUBLIC_KEY_BTC] = {0x04, 0x35, 0x87, 0xCF}; // tpub
        base58Prefixes[EXT_SECRET_KEY_BTC] = {0x04, 0x35, 0x83, 0x94}; // tprv

        bech32Prefixes[PUBKEY_ADDRESS].assign       ("rfalcon",(const char*)"rfalcon"+6);
        bech32Prefixes[SCRIPT_ADDRESS].assign       ("tpr",(const char*)"tpr"+3);
        bech32Prefixes[PUBKEY_ADDRESS_256].assign   ("tpl",(const char*)"tpl"+3);
        bech32Prefixes[SCRIPT_ADDRESS_256].assign   ("tpj",(const char*)"tpj"+3);
        bech32Prefixes[SECRET_KEY].assign           ("tpx",(const char*)"tpx"+3);
        bech32Prefixes[EXT_PUBLIC_KEY].assign       ("tpep",(const char*)"tpep"+4);
        bech32Prefixes[EXT_SECRET_KEY].assign       ("tpex",(const char*)"tpex"+4);
        bech32Prefixes[STEALTH_ADDRESS].assign      ("tps",(const char*)"tps"+3);
        bech32Prefixes[EXT_KEY_HASH].assign         ("tpek",(const char*)"tpek"+4);
        bech32Prefixes[EXT_ACC_HASH].assign         ("tpea",(const char*)"tpea"+4);
        bech32Prefixes[STAKE_ONLY_PKADDR].assign    ("tpcs",(const char*)"tpcs"+4);

        bech32_hrp = "rtpw";

        chainTxData = ChainTxData{
            0,
            0,
            0
        };
    }

    void SetOld()
    {
        genesis = CreateGenesisBlock(1296688602, 2, 0x207fffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        /*
        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        */

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "bcrt";
    }

    /**
     * Allows modifying the Version Bits regtest parameters.
     */
    void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
    }
    void UpdateActivationParametersFromArgs(const ArgsManager& args);
};

void CRegTestParams::UpdateActivationParametersFromArgs(const ArgsManager& args)
{
    if (gArgs.IsArgSet("-segwitheight")) {
        int64_t height = gArgs.GetArg("-segwitheight", consensus.SegwitHeight);
        if (height < -1 || height >= std::numeric_limits<int>::max()) {
            throw std::runtime_error(strprintf("Activation height %ld for segwit is out of valid range. Use -1 to disable segwit.", height));
        } else if (height == -1) {
            LogPrintf("Segwit disabled for testing\n");
            height = std::numeric_limits<int>::max();
        }
        consensus.SegwitHeight = static_cast<int>(height);
    }

    if (!args.IsArgSet("-vbparams")) return;

    for (const std::string& strDeployment : args.GetArgs("-vbparams")) {
        std::vector<std::string> vDeploymentParams;
        boost::split(vDeploymentParams, strDeployment, boost::is_any_of(":"));
        if (vDeploymentParams.size() != 3) {
            throw std::runtime_error("Version bits parameters malformed, expecting deployment:start:end");
        }
        int64_t nStartTime, nTimeout;
        if (!ParseInt64(vDeploymentParams[1], &nStartTime)) {
            throw std::runtime_error(strprintf("Invalid nStartTime (%s)", vDeploymentParams[1]));
        }
        if (!ParseInt64(vDeploymentParams[2], &nTimeout)) {
            throw std::runtime_error(strprintf("Invalid nTimeout (%s)", vDeploymentParams[2]));
        }
        bool found = false;
        for (int j=0; j < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j) {
            if (vDeploymentParams[0] == VersionBitsDeploymentInfo[j].name) {
                UpdateVersionBitsParameters(Consensus::DeploymentPos(j), nStartTime, nTimeout);
                found = true;
                LogPrintf("Setting version bits activation parameters for %s to start=%ld, timeout=%ld\n", vDeploymentParams[0], nStartTime, nTimeout);
                break;
            }
        }
        if (!found) {
            throw std::runtime_error(strprintf("Invalid deployment (%s)", vDeploymentParams[0]));
        }
    }
}

static std::unique_ptr<CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

const CChainParams *pParams() {
    return globalChainParams.get();
};

std::unique_ptr<CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams(gArgs));
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}


void SetOldParams(std::unique_ptr<CChainParams> &params)
{
    if (params->NetworkID() == CBaseChainParams::MAIN) {
        return ((CMainParams*)params.get())->SetOld();
    }
    if (params->NetworkID() == CBaseChainParams::REGTEST) {
        return ((CRegTestParams*)params.get())->SetOld();
    }
};

void ResetParams(std::string sNetworkId, bool fParticlModeIn)
{
    // Hack to pass old unit tests
    globalChainParams = CreateChainParams(sNetworkId);
    if (!fParticlModeIn) {
        SetOldParams(globalChainParams);
    }
};

/**
 * Mutable handle to regtest params
 */
CChainParams &RegtestParams()
{
    return *globalChainParams.get();
};
