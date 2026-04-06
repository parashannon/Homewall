import smtplib
from email.mime.text import MIMEText

def send_gmx_email(
    gmx_username: str,
    gmx_password: str,
    to_address: str,
    subject: str,
    body: str
) -> None:
    """
    Sends a plain-text email via GMX using STARTTLS on port 587.

    :param gmx_username: Your full GMX email address (e.g. 'yourusername@gmx.com')
    :param gmx_password: Your GMX account password           jwcd orik tahq fhll
    :param to_address:   The recipient's email address (e.g. 'somebody@example.com')
    :param subject:      Subject line for the email
    :param body:         Body text of the email (plain text)
    """
    # Create the email message
    msg = MIMEText(body, "plain")
    msg["Subject"] = subject
    msg["From"] = gmx_username
    msg["To"] = to_address

    # GMX SMTP server details for STARTTLS
    smtp_server = "smtp.gmail.com"
    smtp_port = 587  # STARTTLS port

    try:
        # Connect to the GMX SMTP server
        with smtplib.SMTP(smtp_server, smtp_port) as server:
            # Upgrade the connection to TLS
            server.starttls()
            # Log in with your GMX credentials
            server.login(gmx_username, gmx_password)
            # Send the email
            server.send_message(msg)

        print("Email sent successfully via GMX!")
    except Exception as e:
        print(f"Failed to send email via GMX. Error: {e}")


if __name__ == "__main__":
    # -------------------------------------------------------------------------
    # UPDATE THESE VALUES before running:
    # -------------------------------------------------------------------------
    GMX_USERNAME = "shannon.eilers@gmail.com"    # Your GMX email address
    GMX_PASSWORD = "jwcdoriktahqfhll"      # GMX password (or app password if 2FA is enabled)
    TO_ADDRESS   = "4352321110@vtext.com"          # Recipient's email address

    SUBJECT = "Campground Found!"
    BODY    = "Hello from Python, using GMX SMTP!"

    print("Attempting to send email via GMX...")
    send_gmx_email(
        gmx_username=GMX_USERNAME,
        gmx_password=GMX_PASSWORD,
        to_address=TO_ADDRESS,
        subject=SUBJECT,
        body=BODY
    )
