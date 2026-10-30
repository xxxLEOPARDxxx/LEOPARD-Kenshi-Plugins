// Included by core_tests.cpp; exercises the save-file boundary without game objects.
namespace DfStateValidationTests
{
    void Reject(const std::string& text)
    {
        std::vector<DfFactionState> out(1);
        out[0].recordId = "previous-save";
        CHECK(!DfDeserialize(text, out));
        CHECK(out.empty());
    }

    void RejectField(const char* field)
    {
        Reject(std::string("[faction 0]\nid=df\n") + field + "\n");
    }

    void InvalidNumbers()
    {
        const char* fields[] = {
            "foundedHour=nan", "lastTickHour=inf", "event=-inf|found",
            "lastTickHour=1e999", "foundedHour=-1", "lastTickHour=-0.01",
            "event=-1|found", "node=nan|2|3|ore|0", "node=1|inf|3|ore|0",
            "node=1|2|-inf|ore|0", "node=1e100|2|3|ore|0",
            "node=1|2|-1e100|ore|0", "node=1|2|3|ore|-1",
            "node=1|2|3|ore|2147483648", "node=1|2|3|ore|-2147483649",
            "node=1|2|3|ore|99999999999999999999999999999999999",
            "techLevel=2147483648", "techPoints=2147483648", "techLevel=-1",
            "techLevel=11", "techPoints=-1", "techPoints=100",
            "techLevel=3\ntechPoints=400", "techLevel=10\ntechPoints=1"
        };
        for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i)
            RejectField(fields[i]);
    }

    void DuplicateIdsAndBadHeaders()
    {
        Reject("[faction 0]\nid=same\n[faction 1]\nid=same\n");
        Reject("[faction 0]\nid=line\\nID\n[faction 1]\nid=line\\nID\n");
        Reject("[faction ]\nid=df\n");
        Reject("[faction -1]\nid=df\n");
        Reject("[faction 2147483648]\nid=df\n");
        Reject("[faction xyz]\nid=df\n");
        Reject(std::string("[faction 0]\nid=df\nname=a") + char(0) + "b");
    }

    void ExistingFormatSurvives()
    {
        std::vector<DfFactionState> out;
        CHECK(DfDeserialize("", out));
        CHECK(out.empty());
        const std::string text =
            "[faction 0]\r\nid=df\\\\one\r\nname=Line\\nName=a\\r\r\n"
            "race=human\r\nfounded=1\r\nextinct=0\r\nfoundedHour=1.5\r\n"
            "lastTickHour=2.5\r\ntechLevel=3\r\ntechPoints=399\r\n"
            "node=-12.5|0|34.25|ore\\\\id|2147483647\r\n"
            "event=2|cargo=a|b\\nline\r\nfuture_key=opaque\\q|value\r\n"
            "[faction 1]\r\nid=df-two\r\ntechLevel=10\r\ntechPoints=0";
        CHECK(DfDeserialize(text, out));
        CHECK(out.size() == 2);
        if (out.size() == 2)
        {
            CHECK(out[0].recordId == "df\\one");
            CHECK(out[0].name == "Line\nName=a\r");
            CHECK(out[0].foundedHour == 1.5 && out[0].lastTickHour == 2.5);
            CHECK(out[0].techLevel == 3 && out[0].techPoints == 399);
            CHECK(out[0].nodes.size() == 1 && out[0].nodes[0].stored == 2147483647);
            CHECK(out[0].nodes[0].itemSid == "ore\\id");
            CHECK(out[0].nodes[0].x == -12.5f && out[0].nodes[0].z == 34.25f);
            CHECK(out[0].events.size() == 1 && out[0].events[0].text == "cargo=a|b\nline");
            std::vector<DfFactionState> again;
            CHECK(DfDeserialize(DfSerialize(out), again));
            CHECK(again.size() == 2 && again[0].nodes[0].stored == 2147483647);
        }

        std::string events("[faction 0]\nid=df\n");
        for (int i = 0; i < 25; ++i)
        {
            char line[64];
            sprintf_s(line, sizeof(line), "event=%d|event %d\n", i, i);
            events += line;
        }
        CHECK(DfDeserialize(events, out));
        CHECK(out.size() == 1 && out[0].events.size() == 20);
        if (out.size() == 1 && out[0].events.size() == 20)
            CHECK(out[0].events.front().hour == 5 && out[0].events.back().hour == 24);
    }

    void ResourceBounds()
    {
        std::vector<DfFactionState> out;
        Reject(std::string(16 * 1024 * 1024 + 1, '\n'));
        Reject("[faction 0]\nid=df\nfuture=" + std::string(65536, 'x'));
        std::string factions;
        for (int i = 0; i < 1024; ++i)
        {
            char part[96];
            sprintf_s(part, sizeof(part), "[faction %d]\nid=df%d\n", i, i);
            factions += part;
        }
        CHECK(DfDeserialize(factions, out));
        CHECK(out.size() == 1024);
        factions += "[faction 1024]\nid=df1024\n";
        Reject(factions);
        std::string nodes("[faction 0]\nid=df\n");
        for (int i = 0; i < 16384; ++i)
            nodes += "node=1|2|3|ore|0\n";
        CHECK(DfDeserialize(nodes, out));
        CHECK(out.size() == 1 && out[0].nodes.size() == 16384);
        nodes += "node=1|2|3|ore|0\n";
        Reject(nodes);
    }
}

void RunStateValidationTests()
{
    DfStateValidationTests::InvalidNumbers();
    DfStateValidationTests::DuplicateIdsAndBadHeaders();
    DfStateValidationTests::ExistingFormatSurvives();
    DfStateValidationTests::ResourceBounds();
}
