from twilio.rest import Client

# in this part you have to replace account_sid
# auth_token, twilio_number, recipient_number with your actual credential

account_sid = 'account_sid'
auth_token = 'auth_token'
twilio_number = 'twilio_number'
recipient_number = 'recipient_number'

# Create Twilio client
client = Client(account_sid, auth_token)

# Send SMS
# in body part you have to write your message
message = client.messages.create(
    body='This is a new message',
    from_=twilio_number,
    to=recipient_number
)

print(f&quot;Message sent with SID: {message.sid}&quot;)