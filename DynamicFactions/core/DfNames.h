#pragma once
// Уникальные названия динамических фракций. Без заголовков игры.

#include <map>
#include <string>
#include <vector>

// Название из словаря расы: «Прилагательное Существительное» (английское,
// это имя собственное - не переводится). race: "human", "shek", "skeleton",
// "hive"; другое - словарь людей. Одно и то же зерно - одно и то же имя.
// Занятые (taken) пропускаются; не нашлось за 64 попытки - к имени по зерну
// приписывается « II», « III»... пока не станет свободным.
std::string DfMakeName(const std::string& race, unsigned seed, const std::vector<std::string>& taken);

// Словарь названий на языке игры - файл мода names/<язык>.txt (UTF-8, в DLL
// только латиница). Прилагательное согласуется с родом существительного.
//   [human]                       раса
//   noun=Шакалы|p                 род: m, f, n, p (множественное)
//   adj=Железный|Железная|Железное|Железные     формы в том же порядке
//   # комментарий
struct DfNoun
{
    std::string word;
    int gender;                 // 0 m, 1 f, 2 n, 3 p
};

struct DfAdjective
{
    std::string forms[4];
};

struct DfNameList
{
    std::vector<DfAdjective> adj;
    std::vector<DfNoun> nouns;
};

typedef std::map<std::string, DfNameList> DfNameDict;   // по расе

// false - строка не разобралась (out тогда не трогать).
bool DfParseNameDict(const std::string& text, DfNameDict& out);

// Как DfMakeName, но из словаря. Расы нет - словарь людей; словарь пуст -
// английский DfMakeName.
std::string DfMakeNameFrom(const DfNameDict& dict, const std::string& race, unsigned seed,
                           const std::vector<std::string>& taken);
