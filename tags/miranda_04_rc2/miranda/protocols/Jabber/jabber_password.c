/*

Jabber Protocol Plugin for Miranda IM
Copyright (C) 2002-2004  Santithorn Bunchua

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "jabber.h"
#include "jabber_iq.h"
#include "resource.h"

static BOOL CALLBACK JabberChangePasswordDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

int JabberMenuHandleChangePassword(WPARAM wParam, LPARAM lParam)
{
	if (IsWindow(hwndJabberChangePassword))
		SetForegroundWindow(hwndJabberChangePassword);
	else {
		hwndJabberChangePassword = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_CHANGEPASSWORD), NULL, JabberChangePasswordDlgProc, 0);
	}

	return 0;
}

static BOOL CALLBACK JabberChangePasswordDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		{
			char text[128];

			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hInst, MAKEINTRESOURCE(IDI_KEYS)));
			TranslateDialogDefault(hwndDlg);
			if (jabberOnline && jabberThreadInfo!=NULL) {
				_snprintf(text, sizeof(text), "%s %s@%s", Translate("Set New Password for"), jabberThreadInfo->username, jabberThreadInfo->server);
				SetWindowText(hwndDlg, text);
			}
		}
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			if (jabberOnline && jabberThreadInfo!=NULL) {
				char newPasswd[128], text[128];
				char *username, *password, *server;
				int iqId;

				GetDlgItemText(hwndDlg, IDC_NEWPASSWD, newPasswd, sizeof(newPasswd));
				GetDlgItemText(hwndDlg, IDC_NEWPASSWD2, text, sizeof(text));
				if (strcmp(newPasswd, text)) {
					MessageBox(hwndDlg, Translate("New password does not match."), Translate("Change Password"), MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
					break;
				}
				GetDlgItemText(hwndDlg, IDC_OLDPASSWD, text, sizeof(text));
				if (strcmp(text, jabberThreadInfo->password)) {
					MessageBox(hwndDlg, Translate("Current password is incorrect."), Translate("Change Password"), MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
					break;
				}
				if ((server=JabberTextEncode(jabberThreadInfo->server)) != NULL) {
					if ((username=JabberTextEncode(jabberThreadInfo->username)) != NULL) {
						if ((password=JabberTextEncode(newPasswd)) != NULL) {
							strncpy(jabberThreadInfo->newPassword, newPasswd, sizeof(jabberThreadInfo->newPassword));
							iqId = JabberSerialNext();
							JabberIqAdd(iqId, IQ_PROC_NONE, JabberIqResultSetPassword);
							JabberSend(jabberThreadInfo->s, "<iq type='set' id='"JABBER_IQID"%d' to='%s'><query xmlns='jabber:iq:register'><username>%s</username><password>%s</password></query></iq>", iqId, server, username, password);
							free(password);
						}
						free(username);
					}
					free(server);
				}
			}
			DestroyWindow(hwndDlg);
			break;
		case IDCANCEL:
			DestroyWindow(hwndDlg);
			break;
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		break;
	case WM_DESTROY:
		hwndJabberChangePassword = NULL;
		break;
	}

	return FALSE;
}
