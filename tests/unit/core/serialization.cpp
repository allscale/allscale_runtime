#include <allscale/utils/serializer.h>
#include <allscale/utils/serializer/vectors.h>

#include <hpx/util/lightweight_test.hpp>
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>

struct Serializable
{
    Serializable(int j) : i(j) {}
    Serializable() : i(0) {}

    int i;
    static Serializable load(allscale::utils::ArchiveReader& ar)
    {
        Serializable s;
        s.i = ar.read<int>();
        return s;
    }

    void store(allscale::utils::ArchiveWriter& ar) const
    {
        ar.write(i);
    }
};

struct VectorSerializable
{
    VectorSerializable(std::vector<int> j) : i(j) {}
    VectorSerializable() {}

    std::vector<int> i;
    static VectorSerializable load(allscale::utils::ArchiveReader& ar)
    {
        VectorSerializable s;
        s.i = ar.read<std::vector<int>>();
        return s;
    }

    void store(allscale::utils::ArchiveWriter& ar) const
    {
        ar.write(i);
    }
};

int hpx_main(int argc, char **argv)
{
    std::vector<char> buffer;

    static_assert(allscale::utils::is_serializable<Serializable>::value,
        "Serializable class not covered by trait");

    {
        hpx::serialization::output_archive oa(buffer);
        Serializable s(42);
        oa & s;
    }
    {
        hpx::serialization::input_archive ia(buffer);
        Serializable s;
        ia & s;
        HPX_TEST_EQ(s.i, 42);
    }
    {
        hpx::serialization::output_archive oa(buffer);
        std::vector<Serializable> s({42, 57, 1337});
        oa & s;
    }
    {
        hpx::serialization::input_archive ia(buffer);
        std::vector<Serializable> s;
        ia & s;
        HPX_TEST_EQ(s[0].i, 42);
        HPX_TEST_EQ(s[1].i, 57);
        HPX_TEST_EQ(s[2].i, 1337);
    }
    {
        hpx::serialization::output_archive oa(buffer);
        VectorSerializable s({42, 57, 1337});
        oa & s;
    }
    {
        hpx::serialization::input_archive ia(buffer);
        VectorSerializable s;
        ia & s;
        HPX_TEST_EQ(s.i[0], 42);
        HPX_TEST_EQ(s.i[1], 57);
        HPX_TEST_EQ(s.i[2], 1337);
    }
    return hpx::finalize();
}

int main(int argc, char **argv)
{
    hpx::init(argc, argv);

    return hpx::util::report_errors();
}
