﻿#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "common.h"
#include "verify.h"

#define TEMP_FILE "temp_user.txt"
//int updata_file_2(Lend_Return);

void auto_borrow(Book* temp_b, char returnDate[]) {
    bool book_integrity = true;
    linked_list* book_list = read_book_data(&book_integrity);

    bool user_integrity = true;
    linked_list* user_list = read_user_data(&user_integrity);
    User* user = find_by_userId(user_list, temp_b->studentId);

    //대출 가능한지, 사용자 연체여부, 대출 가능 도서 개수 확인
    if (user->isOverdue == 'N' && user->lendAvailable > 0) {
        //해당 사용자의 <대출 중인 도서 BID 목록>에 도서 BID 추가, <대출 가능 도서 개수>, <예약 가능 도서 개수>를 수정
        strcpy(user->lentBids[5 - user->lendAvailable], temp_b->bid);
        user->lendAvailable--;
        user->reserveAvailable++;
        update_file(USER_FILE, user_list);

        //도서 정보 파일 내 해당 도서의 <대출 가능 여부>를 ‘N’, <예약 가능 여부>는 ‘Y'로 수정, <예약중인 사용자 학번>을 삭제
        temp_b->isAvailable = 'N';
        temp_b->isReserveAvailable = 'Y';
        strcpy(temp_b->studentId, "");
        update_file(BOOK_FILE, book_list);

        //대출 및 반납 기록 파일에서 해당 사용자의 ID, 입력한 BID, 대출 날짜(대출 날짜는 전 이용자의 반납날짜로 저장), 반납 날짜(대출 중이므로 미반납 상태인 ‘ - ’ 저장)순으로 저장
        bool lend_integrity = true;
        linked_list* lend_list = read_borrow_data(&lend_integrity);
        Lend_Return* new_lend = (Lend_Return*)malloc(sizeof(Lend_Return));

        strcpy(new_lend->userid, user->studentId);
        strcpy(new_lend->bookBid, temp_b->bid);
        strcpy(new_lend->borrowDate, returnDate);
        strcpy(new_lend->returnDate, "");

        insert_back(lend_list, new_lend);
        update_file(LEND_RETURN_FILE, lend_list);
        free(new_lend);
        //무결성 처리??
    }
    else {
        //도서 정보 파일 내 해당 도서의 <예약 가능 여부>를 ‘Y’로 바꾸고 <예약 중인 사용자 학번 > 을 삭제하며 예약이 취소됩니다.
        temp_b->isReserveAvailable = 'Y';
        strcpy(temp_b->studentId, "");

        update_file(BOOK_FILE, book_list);
    }

    return;
}



void run_return() {
    if (!is_logged_in) {
        printf("You must login first to return books.\n");
        return;
    }
    if (current_user.lendAvailable == 5) {
        printf("You have no books to return.\n");
        return;
    }
    printf("\n[Here is the list of books you have borrowed]\n");
    bool user_integrity = true;
    linked_list* user_list = read_user_data(&user_integrity);
    if (!user_integrity) {
        print_list(user_list, 5);
        return;
    }
    bool book_integrity = true;
    linked_list* book_list = read_book_data(&book_integrity);
    if (!book_integrity) {
        print_list(book_list, 4);
        return;
    }
    User* temp_u = NULL;
    Book* temp_b = NULL;
    Lend_Return* temp_l = NULL;
    node* current_u = user_list->head;
    while (current_u) {
        User* user = (User*)current_u->data;
        if (!strcmp(user->studentId, current_user.studentId)) {
            temp_u = user;
            for (int i = 0; i < 5 - (user->lendAvailable); i++) {
                node* current_b = book_list->head;
                while (current_b) {
                    Book* book = (Book*)current_b->data;
                    if (!strcmp(book->bid, user->lentBids[i])) {
                        printf("> Title: %s\n  Author: %s\n  BID: %s\n\n", book->title, book->author, book->bid);
                    }
                    current_b = current_b->next;
                }
            }
            break;
        }
        current_u = current_u->next;
    }

    char bid_input[MAX_BID];
    while (1) {
        printf("Enter BID of the book to return > ");
        fgets(bid_input, sizeof(bid_input), stdin);
        bid_input[strcspn(bid_input, "\n")] = '\0';

        // BID 유효성 검사
        if (is_valid_bid(bid_input) == 7) {
            printf(".!! Error: BID cannot be an empty string\n");
            continue;
        }

        if (is_valid_bid(bid_input) == 8) {
            printf(".!! Error: BID cannot consist of only whitespace characters\n");
            continue;
        }

        if (is_valid_bid(bid_input) == 9) {
            printf(".!! Error: A tab character cannot be placed between the first and last valid characters of the BID\n");
            continue;
        }

        trim(bid_input);
        int found = -1;
        for (int i = 0; i < 5 - (temp_u->lendAvailable); i++) {
            if (!strcmp(bid_input, temp_u->lentBids[i])) {
                found = i;
                break;
            }
        }
        temp_b = find_by_bid(book_list, bid_input);
        if (found == -1) {
            printf("Error: This book is not in your borrowed list.\n");
            continue;
        }
        else {
            for (int i = found; i < 4 - (temp_u->lendAvailable); i++) {
                strcpy(temp_u->lentBids[i], temp_u->lentBids[i + 1]);
            }
            temp_u->lentBids[4 - (temp_u->lendAvailable)][0] = '\0';
            break;
        }

    }
    bool lend_integrity = true;
    linked_list* lend_list = read_borrow_data(&lend_integrity);
    if (!lend_integrity) {
        print_list(lend_list, 5);
        printf("here Error");
        return;
    }
    node* current_l = lend_list->head;
    while (current_l) {
        Lend_Return* lend = (Lend_Return*)current_l->data;
        if (!strcmp(lend->bookBid, temp_b->bid)) {
            temp_l = lend;
        }
        current_l = current_l->next;
    }
    char return_date[MAX_DATE];
    while (1)
    {
        printf("Enter return date > ");
        fgets(return_date, sizeof(return_date), stdin);
        trim(return_date);

        if (!is_valid_date(return_date)) {
            continue;
        }

        strcpy(temp_l->returnDate, return_date);
        //temp_l->isOverdue = 'N';
        break;
    }
    printf("\n[He following book matches the entered BID]\n");

    printf("> Title: %s\n  Author: %s\n  BID: %s\n\n", temp_b->title, temp_b->author, temp_b->bid);

    printf("Do you really want to return this book? (Enter to confirm / No to cancel) > ");
    char confirm[5];
    fgets(confirm, sizeof(confirm), stdin);
    trim(confirm);

    if (check_input(confirm) == 0) {
        printf("Returning cancelled.\n");
        return;
    }
    else {
        temp_u->lendAvailable++;
        temp_b->isAvailable = 'Y';
        if (temp_u->reserveAvailable == 'N')
            auto_borrow(temp_b, return_date);
    }

    if (!update_file(USER_FILE, user_list) || !update_file(BOOK_FILE, book_list) || !update_file(LEND_RETURN_FILE, lend_list)) {
        return;
    }
}