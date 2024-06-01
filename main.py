import uuid
from flask import Flask, url_for, render_template, request, jsonify
import sqlite
import qrcode

app = Flask(__name__)

api_keys = {
    "key": "key_name"
}

@app.route('/')
def index():
    reservations = sqlite.get_all()
    qr_codes = []

    for reservation in reservations:
        qr_codes.append({
            'reservation_id': reservation[0],
            'qr_code_url': url_for('static', filename=f'qr_codes/{reservation[0]}.png')
        })

    return render_template('index.html')


@app.route('/reservation', methods=["POST"])
def create_reservation():
    
    api_key = request.headers.get("X-API-Key")

    if api_key not in api_keys:
       return jsonify({'error': 'Clé API non valide.'}), 401

    data = request.get_json()
    reservation_id = str(uuid.uuid4())[:8]

    sqlite.create(reservation_id, data["product_id"], data["quantity"])
    print(sqlite.get_all())

    qr = qrcode.QRCode(
        version=1,
        error_correction=qrcode.constants.ERROR_CORRECT_L,
        box_size=10,
        border=4,
    )
    qr.add_data(reservation_id)
    qr.make(fit=True)

    img = qr.make_image(fill_color="black", back_color="white")
    img.save(f"static/qr_codes/{reservation_id}.png")

    return jsonify({'reservation_id': reservation_id}), 201


@app.route('/distribution/<string:reservation_id>')
def check_reservation(reservation_id):
    
    api_key = request.headers.get("X-API-Key")

    if api_key not in api_keys:
        return jsonify({'error': 'Clé API non valide.'}), 401

    if sqlite.get_from_id(reservation_id):
        return jsonify({'exists': True, 'product_id': sqlite.get_from_id(reservation_id)[1],
                        'quantity': sqlite.get_from_id(reservation_id)[2]}), 200
    else:
        return jsonify({'exists': False}), 404


if __name__ == '__main__':
    app.run(host='0.0.0.0', debug=True)
