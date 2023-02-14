
#include "core/core.h"

#define METADOT_CREFLECT_IMPL

#include "code_reflection.h"
#include "engine/engine_meta.h"

#include <stdio.h>

METADOT_CREFLECT_DEFINE_STRUCT(TestStruct, (INTEGER, int, int_field),
                               (STRING, char, array_field, 20), (INTEGER, size_t, size_field))

/* 
 *  struct defined for test
 */
struct YearRecord
{
    int year;
    CSTR yearLabel;
    DECLARE_LIST(struct MonthRecord {
        int month;
        CSTR monthLabel;
        DECLARE_LIST(struct DayRecord {
            int day;
            CSTR dayLabel;
            R_bool mark;
        })
        dayList;
    })
    monthList;
};

/* 
 *  you need to define metainfo for 'YearRecord' with 'YearRecord' together
 */
DEFINE_METAINFO(YearRecord) {
    METAINFO_CREATE(YearRecord);

    METAINFO_ADD_MEMBER(YearRecord, FIELD_TYPE_INT, year);
    METAINFO_ADD_MEMBER(YearRecord, FIELD_TYPE_CSTR, yearLabel);

    METAINFO_CHILD_LIST_BEGIN(YearRecord, MonthRecord, monthList);

    METAINFO_ADD_MEMBER(MonthRecord, FIELD_TYPE_INT, month);
    METAINFO_ADD_MEMBER(MonthRecord, FIELD_TYPE_CSTR, monthLabel);

    METAINFO_CHILD_LIST_BEGIN(MonthRecord, DayRecord, dayList);

    METAINFO_ADD_MEMBER(DayRecord, FIELD_TYPE_INT, day);
    METAINFO_ADD_MEMBER(DayRecord, FIELD_TYPE_CSTR, dayLabel);
    METAINFO_ADD_MEMBER(DayRecord, FIELD_TYPE_BOOL, mark);

    METAINFO_CHILD_END();

    METAINFO_CHILD_END();
}

int main() {
    /* 
	*  remember to register metainfo of 'YearRecord' in your implement code
	*/
    REGISTER_METAINFO(YearRecord);

    {
        CSTR json;
        struct YearRecord *rec = OBJECT_NEW(YearRecord);

        rec->year = 2021;
        rec->yearLabel = CS("Here is yearLabel");// CSTR must be initialized with 'CS' macro

        {
            // you can add an empty object into list first
            LIST_ADD_EMPTY(rec->monthList, MonthRecord);
            rec->monthList.array[0].month = 11;
            rec->monthList.array[0].monthLabel = CS("Nov.");

            LIST_ADD_EMPTY(rec->monthList.array[0].dayList, DayRecord);
            LIST_ADD_EMPTY(rec->monthList.array[0].dayList, DayRecord);
            LIST_ADD_EMPTY(rec->monthList.array[0].dayList, DayRecord);
            rec->monthList.array[0].dayList.array[2].day = 4;
            rec->monthList.array[0].dayList.array[2].dayLabel = CS("Today nothing to do.");
            rec->monthList.array[0].dayList.array[2].mark = 1;
        }

        {
            // also object that is on stack can be add into list
            // make sure that the object must zero memory
            struct MonthRecord month_rec = {0};
            month_rec.month = 12;
            month_rec.monthLabel = CS("next month");
            LIST_ADD_OBJ(rec->monthList, month_rec);
        }

        // convert to json string
        json = OBJECT_TO_JSON(rec, YearRecord);
        printf(json.cstr);

        // clear object but not free
        OBJECT_CLEAR(rec, YearRecord);

        // delete 'rec' (free allocated memory), instead of 'free' function
        OBJECT_DELETE(rec, YearRecord);

        {
            // convert from json string
            struct YearRecord *rec2 = OBJECT_FROM_JSON(YearRecord, json.cstr);

            OBJECT_DELETE(rec2, YearRecord);
        }

        // the output json string is CSTR type, do not forget to free it!
        CS_CLEAR(json);
    }

    getchar();

    METADOT_CREFLECT_TypeInfo *info = metadot_creflect_get_TestStruct_type_info();

    for (size_t i = 0; i < info->fields_count; i++) {
        METADOT_CREFLECT_FieldInfo *field = &info->fields[i];
        printf(" * %s\n", field->field_name);
        printf("    Type: %s\n", field->field_type);
        printf("    Total size: %lu\n", field->size);
        printf("    Offset: %lu\n", field->offset);
        if (field->array_size != -1) {
            printf("    It is an array! Number of elements: %d, size of single element: %lu\n",
                   field->array_size, field->size / field->array_size);
        }
    }

    getchar();
    return 0;
}
