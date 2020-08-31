// Copyright (c) 2017-2019 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/setup_common.h>
#include <net.h>
#include <script/signingprovider.h>
#include <script/script.h>
#include <consensus/validation.h>
#include <consensus/merkle.h>
#include <consensus/tx_verify.h>
#include <key/extkey.h>
#include <pos/kernel.h>
#include <chainparams.h>

#include <script/sign.h>
#include <policy/policy.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(falconchain_tests, ParticlBasicTestingSetup)


BOOST_AUTO_TEST_CASE(oldversion_test)
{
    CBlock blk, blkOut;
    blk.nTime = 1487406900;

    CMutableTransaction txn;
    blk.vtx.push_back(MakeTransactionRef(txn));

    CDataStream ss(SER_DISK, 0);

    ss << blk;
    ss >> blkOut;

    BOOST_CHECK(blk.vtx[0]->nVersion == blkOut.vtx[0]->nVersion);
}

BOOST_AUTO_TEST_CASE(signature_test)
{
    SeedInsecureRand();
    FillableSigningProvider keystore;

    CKey k;
    InsecureNewKey(k, true);
    keystore.AddKey(k);

    CPubKey pk = k.GetPubKey();
    CKeyID id = pk.GetID();

    CMutableTransaction txn;
    txn.nVersion = GHOST_TXN_VERSION;
    txn.nLockTime = 0;

    int nBlockHeight = 22;
    OUTPUT_PTR<CTxOutData> out0 = MAKE_OUTPUT<CTxOutData>();
    out0->vData = SetCompressedInt64(out0->vData, nBlockHeight);
    txn.vpout.push_back(out0);

    CScript script = CScript() << OP_DUP << OP_HASH160 << ToByteVector(id) << OP_EQUALVERIFY << OP_CHECKSIG;
    OUTPUT_PTR<CTxOutStandard> out1 = MAKE_OUTPUT<CTxOutStandard>();
    out1->nValue = 10000;
    out1->scriptPubKey = script;
    txn.vpout.push_back(out1);

    CMutableTransaction txn2;
    txn2.nVersion = GHOST_TXN_VERSION;
    txn2.vin.push_back(CTxIn(txn.GetHash(), 0));

    std::vector<uint8_t> vchAmount(8);
    memcpy(&vchAmount[0], &out1->nValue, 8);

    SignatureData sigdata;
    BOOST_CHECK(ProduceSignature(keystore, MutableTransactionSignatureCreator(&txn2, 0, vchAmount, SIGHASH_ALL), script, sigdata));

    ScriptError serror = SCRIPT_ERR_OK;
    BOOST_CHECK(VerifyScript(txn2.vin[0].scriptSig, out1->scriptPubKey, &sigdata.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, MutableTransactionSignatureChecker(&txn2, 0, vchAmount), &serror));
    BOOST_CHECK(serror == SCRIPT_ERR_OK);
}

BOOST_AUTO_TEST_CASE(falconchain_test)
{
    SeedInsecureRand();
    FillableSigningProvider keystore;

    CKey k;
    InsecureNewKey(k, true);
    keystore.AddKey(k);

    CPubKey pk = k.GetPubKey();
    CKeyID id = pk.GetID();

    CScript script = CScript() << OP_DUP << OP_HASH160 << ToByteVector(id) << OP_EQUALVERIFY << OP_CHECKSIG;

    CBlock blk;
    blk.nVersion = GHOST_BLOCK_VERSION;
    blk.nTime = 1487406900;

    CMutableTransaction txn;
    txn.nVersion = GHOST_TXN_VERSION;
    txn.SetType(TXN_COINBASE);
    txn.nLockTime = 0;
    OUTPUT_PTR<CTxOutStandard> out0 = MAKE_OUTPUT<CTxOutStandard>();
    out0->nValue = 10000;
    out0->scriptPubKey = script;
    txn.vpout.push_back(out0);


    blk.vtx.push_back(MakeTransactionRef(txn));

    bool mutated;
    blk.hashMerkleRoot = BlockMerkleRoot(blk, &mutated);
    blk.hashWitnessMerkleRoot = BlockWitnessMerkleRoot(blk, &mutated);


    CDataStream ss(SER_DISK, 0);
    ss << blk;

    CBlock blkOut;
    ss >> blkOut;

    BOOST_CHECK(blk.hashMerkleRoot == blkOut.hashMerkleRoot);
    BOOST_CHECK(blk.hashWitnessMerkleRoot == blkOut.hashWitnessMerkleRoot);
    BOOST_CHECK(blk.nTime == blkOut.nTime && blkOut.nTime == 1487406900);

    BOOST_CHECK(TXN_COINBASE == blkOut.vtx[0]->GetType());

    CMutableTransaction txnSpend;

    txnSpend.nVersion = GHOST_BLOCK_VERSION;
}

BOOST_AUTO_TEST_CASE(varints)
{
    // encode

    uint8_t c[128];
    std::vector<uint8_t> v;

    size_t size = 0;
    for (int i = 0; i < 100000; i++) {
        size_t sz = GetSizeOfVarInt<VarIntMode::NONNEGATIVE_SIGNED>(i);
        BOOST_CHECK(sz = PutVarInt(c, i));
        BOOST_CHECK(0 == PutVarInt(v, i));
        BOOST_CHECK(0 == memcmp(c, &v[size], sz));
        size += sz;
        BOOST_CHECK(size == v.size());
    }

    for (uint64_t i = 0;  i < 100000000000ULL; i += 999999937) {
        BOOST_CHECK(0 == PutVarInt(v, i));
        size += GetSizeOfVarInt<VarIntMode::DEFAULT>(i);
        BOOST_CHECK(size == v.size());
    }


    // decode
    size_t nB = 0, o = 0;
    for (int i = 0; i < 100000; i++) {
        uint64_t j = -1;
        BOOST_CHECK(0 == GetVarInt(v, o, j, nB));
        BOOST_CHECK_MESSAGE(i == (int)j, "decoded:" << j << " expected:" << i);
        o += nB;
    }

    for (uint64_t i = 0;  i < 100000000000ULL; i += 999999937) {
        uint64_t j = -1;
        BOOST_CHECK(0 == GetVarInt(v, o, j, nB));
        BOOST_CHECK_MESSAGE(i == j, "decoded:" << j << " expected:" << i);
        o += nB;
    }
}

BOOST_AUTO_TEST_CASE(mixed_input_types)
{
    CMutableTransaction txn;
    txn.nVersion = GHOST_TXN_VERSION;
    BOOST_CHECK(txn.IsFalconVersion());

    CAmount txfee;
    int nSpendHeight = 1;
    CCoinsView viewDummy;
    CCoinsViewCache inputs(&viewDummy);

    CMutableTransaction txnPrev;
    txnPrev.nVersion = GHOST_TXN_VERSION;
    BOOST_CHECK(txnPrev.IsFalconVersion());

    CScript scriptPubKey;
    txnPrev.vpout.push_back(MAKE_OUTPUT<CTxOutStandard>(1 * COIN, scriptPubKey));
    txnPrev.vpout.push_back(MAKE_OUTPUT<CTxOutStandard>(2 * COIN, scriptPubKey));
    txnPrev.vpout.push_back(MAKE_OUTPUT<CTxOutCT>());
    txnPrev.vpout.push_back(MAKE_OUTPUT<CTxOutCT>());

    CTransaction txnPrev_c(txnPrev);
    AddCoins(inputs, txnPrev_c, 1);

    uint256 prevHash = txnPrev_c.GetHash();

    std::vector<std::pair<std::vector<int>, bool> > tests =
    {
        std::make_pair( (std::vector<int>) {0 }, true),
        std::make_pair( (std::vector<int>) {0, 1}, true),
        std::make_pair( (std::vector<int>) {0, 2}, false),
        std::make_pair( (std::vector<int>) {0, 1, 2}, false),
        std::make_pair( (std::vector<int>) {2}, true),
        std::make_pair( (std::vector<int>) {2, 3}, true),
        std::make_pair( (std::vector<int>) {2, 3, 1}, false),
        std::make_pair( (std::vector<int>) {-1}, true),
        std::make_pair( (std::vector<int>) {-1, -1}, true),
        std::make_pair( (std::vector<int>) {2, -1}, false),
        std::make_pair( (std::vector<int>) {0, -1}, false),
        std::make_pair( (std::vector<int>) {0, 0, -1}, false),
        std::make_pair( (std::vector<int>) {0, 2, -1}, false)
    };

    for (auto t : tests) {
        txn.vin.clear();

        for (auto ti : t.first) {
            if (ti < 0)  {
                CTxIn ai;
                ai.prevout.n = COutPoint::ANON_MARKER;
                txn.vin.push_back(ai);
                continue;
            }
            txn.vin.push_back(CTxIn(prevHash, ti));
        }

        CTransaction tx_c(txn);
        CValidationState state;
        Consensus::CheckTxInputs(tx_c, state, inputs, nSpendHeight, txfee);

        if (t.second) {
            BOOST_CHECK(state.GetRejectReason() != "mixed-input-types");
        } else {
            BOOST_CHECK(state.GetRejectReason() == "mixed-input-types");
        }
    }
}
//Test block reward over the years on GHOST
BOOST_AUTO_TEST_CASE(blockreward_at_height_test)
{
    const int64_t nBlocksPerYear = (365 * 24 * 60 * 60) / Params().GetTargetSpacing();
    /* Uncomment this for getting correct vals after a blockreward change
       for(int i=0;i<51;i++){
        BOOST_MESSAGE("    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * " + std::to_string(i) + "), " + std::to_string(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * i)) + ");");
    }
    */
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 0), 2000000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 1), 2000000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 2), 1900000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 3), 1800000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 4), 1720000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 5), 1620000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 6), 1540000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 7), 1480000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 8), 1400000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 9), 1320000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 10), 1260000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 11), 1200000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 12), 1140000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 13), 1080000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 14), 1020000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 15), 980000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 16), 920000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 17), 880000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 18), 840000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 19), 800000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 20), 760000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 21), 720000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 22), 680000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 23), 640000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 24), 620000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 25), 580000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 26), 560000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 27), 520000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 28), 500000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 29), 480000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 30), 460000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 31), 420000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 32), 400000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 33), 380000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 34), 360000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 35), 340000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 36), 340000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 37), 320000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 38), 300000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 39), 280000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 40), 280000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 41), 260000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 42), 240000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 43), 240000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 44), 220000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 45), 200000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 46), 200000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 47), 200000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 48), 200000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 49), 200000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtHeight(nBlocksPerYear * 50), 200000000);
}

BOOST_AUTO_TEST_CASE(blockreward_at_year_test)
{
    /* Uncomment this for getting correct vals after a blockreward change
    for(int i=0;i<51;i++){
        BOOST_MESSAGE("    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(" + std::to_string(i) + "), " + std::to_string(Params().GetProofOfStakeRewardAtYear(i)) + ");");
    }
    */
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(0), 2000000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(1), 2000000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(2), 1900000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(3), 1800000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(4), 1720000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(5), 1620000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(6), 1540000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(7), 1480000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(8), 1400000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(9), 1320000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(10), 1260000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(11), 1200000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(12), 1140000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(13), 1080000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(14), 1020000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(15), 980000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(16), 920000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(17), 880000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(18), 840000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(19), 800000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(20), 760000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(21), 720000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(22), 680000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(23), 640000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(24), 620000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(25), 580000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(26), 560000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(27), 520000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(28), 500000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(29), 480000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(30), 460000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(31), 420000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(32), 400000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(33), 380000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(34), 360000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(35), 340000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(36), 340000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(37), 320000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(38), 300000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(39), 280000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(40), 280000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(41), 260000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(42), 240000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(43), 240000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(44), 220000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(45), 200000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(46), 200000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(47), 200000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(48), 200000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(49), 200000000);
    BOOST_CHECK_EQUAL(Params().GetProofOfStakeRewardAtYear(50), 200000000);
}

BOOST_AUTO_TEST_SUITE_END()
